// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/ConvertToMDMinMaxLocal.h"

#include <cfloat>

#include "MantidKernel/ArrayProperty.h"

#include "MantidMDAlgorithms/ConvToMDSelector.h"
#include "MantidMDAlgorithms/MDWSTransform.h"
#include "MantidMDAlgorithms/UnitsConversionHelper.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid::MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToMDMinMaxLocal)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string ConvertToMDMinMaxLocal::name() const { return "ConvertToMDMinMaxLocal"; }

//----------------------------------------------------------------------------------------------
void ConvertToMDMinMaxLocal::init() {
  ConvertToMDParent::init();

  declareProperty(std::make_unique<Kernel::ArrayProperty<double>>("MinValues", Direction::Output));
  declareProperty(std::make_unique<Kernel::ArrayProperty<double>>("MaxValues", Direction::Output));
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */

void ConvertToMDMinMaxLocal::exec() {

  // -------- get Input workspace
  Mantid::API::MatrixWorkspace_sptr InWS2D = getProperty("InputWorkspace");

  // Collect and Analyze the requests to the job, specified by the input
  // parameters:
  // a) Q selector:
  std::string QModReq = getProperty("QDimensions");
  // b) the energy exchange mode
  std::string dEModReq = getProperty("dEAnalysisMode");
  // c) other dim property;
  std::vector<std::string> otherDimNames = getProperty("OtherDimensions");
  // d) The output dimensions in the Q3D mode, processed together with
  // QConversionScales
  std::string QFrame = getProperty("Q3DFrames");
  // e) part of the procedure, specifying the target dimensions units. Currently
  // only Q3D target units can be converted to different flavors of hkl
  std::string convertTo_ = getProperty("QConversionScales");

  // Build the target ws description as function of the input & output ws and
  // the parameters, supplied to the algorithm
  MDWSDescription targWSDescr;

  // get raw pointer to Q-transformation (do not delete this pointer, it's held
  // by MDTransfFactory!)
  MDTransfInterface *pQtransf = MDTransfFactory::Instance().create(QModReq).get();
  // get number of dimensions this Q transformation generates from the
  // workspace.
  auto iEmode = Kernel::DeltaEMode::fromString(dEModReq);
  // get total number of dimensions the workspace would have.
  unsigned int nMatrixDim = pQtransf->getNMatrixDimensions(iEmode, InWS2D);
  // total number of dimensions
  size_t nDim = nMatrixDim + otherDimNames.size();

  std::vector<double> MinValues, MaxValues;
  MinValues.resize(nDim, -FLT_MAX / 10);
  MaxValues.resize(nDim, FLT_MAX / 10);
  // verify that the number min/max values is equivalent to the number of
  // dimensions defined by properties and min is less max
  targWSDescr.setMinMax(MinValues, MaxValues);
  targWSDescr.buildFromMatrixWS(InWS2D, QModReq, dEModReq, otherDimNames);
  // add expInfoIndex to the target workspace description for further usage as the
  // identifier for the events, which come from this run.
  targWSDescr.addProperty("EXP_INFO_INDEX", uint16_t(0), true);

  // instantiate class, responsible for defining Mslice-type projection
  MDAlgorithms::MDWSTransform MsliceProj;
  // identify if u,v are present among input parameters and use defaults if not
  std::vector<double> ut = getProperty("UProj");
  std::vector<double> vt = getProperty("VProj");
  std::vector<double> wt = getProperty("WProj");
  try {
    // otherwise input uv are ignored -> later it can be modified to set ub
    // matrix if no given, but this may over-complicate things.
    MsliceProj.setUVvectors(ut, vt, wt);
  } catch (std::invalid_argument &) {
    g_log.error() << "The projections are coplanar. Will use defaults "
                     "[1,0,0],[0,1,0] and [0,0,1]\n";
  }

  // set up target coordinate system and identify/set the (multi) dimension's
  // names to use
  targWSDescr.m_RotMatrix = MsliceProj.getTransfMatrix(targWSDescr, QFrame, convertTo_);

  // preprocess detectors (or make fake detectors in CopyMD case)
  targWSDescr.m_PreprDetTable =
      this->preprocessDetectorsPositions(InWS2D, dEModReq, false, std::string(getProperty("PreprocDetectorsWS")));

  // do the job
  findMinMaxValues(targWSDescr, pQtransf, iEmode, MinValues, MaxValues);

  setProperty("MinValues", MinValues);
  setProperty("MaxValues", MaxValues);
}

void ConvertToMDMinMaxLocal::findMinMaxValues(MDWSDescription &WSDescription, MDTransfInterface *const pQtransf,
                                              Kernel::DeltaEMode::Type iEMode, std::vector<double> &MinValues,
                                              std::vector<double> &MaxValues) {

  MDAlgorithms::UnitsConversionHelper unitsConverter;
  double signal(1), errorSq(1);
  //
  size_t nDims = MinValues.size();
  MinValues.assign(nDims, DBL_MAX);
  MaxValues.assign(nDims, -DBL_MAX);
  //
  auto inWS = WSDescription.getInWS();
  std::string convUnitsID = pQtransf->inputUnitID(iEMode, inWS);
  // initialize units conversion
  unitsConverter.initialize(WSDescription, convUnitsID);
  // initialize MD transformation
  pQtransf->initialize(WSDescription);

  //
  auto nSpectra = WSDescription.m_PreprDetTable->getLogs()->getPropertyValueAsType<uint32_t>("ActualDetectorsNum");
  auto detIDMap = WSDescription.m_PreprDetTable->getColVector<size_t>("detIDMap");
  auto sp2detMap = WSDescription.m_PreprDetTable->getColVector<size_t>("spec2detMap");

  // vector to place transformed coordinates;
  std::vector<coord_t> locCoord(nDims);

  pQtransf->calcGenericVariables(locCoord, nDims);
  // PRAGMA_OMP(parallel for reduction(||:rangeChanged))
  for (size_t i = 0; i < nSpectra; i++) {
    // get valid spectrum number
    size_t iSpctr = detIDMap[i];

    // update unit conversion according to current spectra
    unitsConverter.updateConversion(i);
    // update coordinate transformation according to the spectra
    pQtransf->calcYDepCoordinates(locCoord, i);

    // get the range of the input data in the spectra
    auto source_range = inWS->getSpectrum(iSpctr).getXDataRange();

    // extract part of this range which has well defined unit conversion
    source_range = unitsConverter.getConversionRange(source_range.first, source_range.second);

    double x1 = unitsConverter.convertUnits(source_range.first);
    double x2 = unitsConverter.convertUnits(source_range.second);

    std::vector<double> range = pQtransf->getExtremumPoints(x1, x2, i);
    // transform coordinates
    for (double &k : range) {

      pQtransf->calcMatrixCoord(k, locCoord, signal, errorSq);
      // identify min-max ranges for current spectrum
      for (size_t j = 0; j < nDims; j++) {
        if (locCoord[j] < MinValues[j])
          MinValues[j] = locCoord[j];
        if (locCoord[j] > MaxValues[j])
          MaxValues[j] = locCoord[j];
      }
    }
  }
}
} // namespace Mantid::MDAlgorithms
