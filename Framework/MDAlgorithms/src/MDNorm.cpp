﻿// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/MDNorm.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidGeometry/MDGeometry/MDFrameFactory.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/UnitLabelTypes.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include <boost/lexical_cast.hpp>
#include <iostream>

namespace Mantid::MDAlgorithms {

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

namespace {
using VectorDoubleProperty = Kernel::PropertyWithValue<std::vector<double>>;
// function to  compare two intersections (h,k,l,Momentum) by Momentum
bool compareMomentum(const std::array<double, 4> &v1, const std::array<double, 4> &v2) { return (v1[3] < v2[3]); }

// k=sqrt(energyToK * E)
constexpr double energyToK = 8.0 * M_PI * M_PI * PhysicalConstants::NeutronMass * PhysicalConstants::meV * 1e-20 /
                             (PhysicalConstants::h * PhysicalConstants::h);

// compare absolute values of doubles
static bool abs_compare(double a, double b) { return (std::fabs(a) < std::fabs(b)); }
} // namespace

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MDNorm)

//----------------------------------------------------------------------------------------------
/**
 * Constructor
 */
MDNorm::MDNorm()
    : m_normWS(), m_inputWS(), m_isRLU(false), m_UB(3, 3, true), m_W(3, 3, true), m_transformation(), m_hX(), m_kX(),
      m_lX(), m_eX(), m_hIdx(-1), m_kIdx(-1), m_lIdx(-1), m_eIdx(-1), m_numExptInfos(0), m_Ei(0.0), m_diffraction(true),
      m_accumulate(false), m_dEIntegrated(true), m_samplePos(), m_beamDir(), convention("") {}

/// Algorithms name for identification. @see Algorithm::name
const std::string MDNorm::name() const { return "MDNorm"; }

/// Algorithm's version for identification. @see Algorithm::version
int MDNorm::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string MDNorm::category() const { return "MDAlgorithms\\Normalisation"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string MDNorm::summary() const {
  return "Bins multidimensional data and calculate the normalization on the "
         "same grid";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void MDNorm::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<API::IMDEventWorkspace>>("InputWorkspace", "", Kernel::Direction::Input),
      "An input MDEventWorkspace. Must be in Q_sample frame.");

  declareProperty(std::make_unique<WorkspaceProperty<API::IMDEventWorkspace>>(
                      "BackgroundWorkspace", "", Kernel::Direction::Input, PropertyMode::Optional),
                  "An (optional) input MDEventWorkspace for background.  Must be in Q_lab frame.");

  // RLU and settings
  declareProperty("RLU", true, "Use reciprocal lattice units. If false, use Q_sample");
  setPropertyGroup("RLU", "Q projections RLU");

  auto mustBe3D = std::make_shared<Kernel::ArrayLengthValidator<double>>(3);
  std::vector<double> Q0(3, 0.), Q1(3, 0), Q2(3, 0);
  Q0[0] = 1.;
  Q1[1] = 1.;
  Q2[2] = 1.;

  declareProperty(std::make_unique<ArrayProperty<double>>("QDimension0", Q0, mustBe3D),
                  "The first Q projection axis - Default is (1,0,0)");
  setPropertySettings("QDimension0", std::make_unique<Kernel::VisibleWhenProperty>("RLU", IS_EQUAL_TO, "1"));
  setPropertyGroup("QDimension0", "Q projections RLU");

  declareProperty(std::make_unique<ArrayProperty<double>>("QDimension1", Q1, mustBe3D),
                  "The second Q projection axis - Default is (0,1,0)");
  setPropertySettings("QDimension1", std::make_unique<Kernel::VisibleWhenProperty>("RLU", IS_EQUAL_TO, "1"));
  setPropertyGroup("QDimension1", "Q projections RLU");

  declareProperty(std::make_unique<ArrayProperty<double>>("QDimension2", Q2, mustBe3D),
                  "The thirdtCalculateCover Q projection axis - Default is (0,0,1)");
  setPropertySettings("QDimension2", std::make_unique<Kernel::VisibleWhenProperty>("RLU", IS_EQUAL_TO, "1"));
  setPropertyGroup("QDimension2", "Q projections RLU");

  // vanadium
  auto fluxValidator = std::make_shared<CompositeValidator>();
  fluxValidator->add<InstrumentValidator>();
  fluxValidator->add<CommonBinsValidator>();
  auto solidAngleValidator = fluxValidator->clone();
  declareProperty(std::make_unique<WorkspaceProperty<>>("SolidAngleWorkspace", "", Direction::Input,
                                                        API::PropertyMode::Optional, solidAngleValidator),
                  "An input workspace containing integrated vanadium "
                  "(a measure of the solid angle).\n"
                  "Mandatory for diffraction, optional for direct geometry inelastic");
  declareProperty(std::make_unique<WorkspaceProperty<>>("FluxWorkspace", "", Direction::Input,
                                                        API::PropertyMode::Optional, fluxValidator),
                  "An input workspace containing momentum dependent flux.\n"
                  "Mandatory for diffraction. No effect on direct geometry inelastic");
  setPropertyGroup("SolidAngleWorkspace", "Vanadium normalization");
  setPropertyGroup("FluxWorkspace", "Vanadium normalization");

  // Define slicing
  for (std::size_t i = 0; i < 6; i++) {
    std::string propName = "Dimension" + Strings::toString(i) + "Name";
    std::string propBinning = "Dimension" + Strings::toString(i) + "Binning";
    std::string defaultName = "";
    if (i < 3) {
      defaultName = "QDimension" + Strings::toString(i);
    }
    declareProperty(std::make_unique<PropertyWithValue<std::string>>(propName, defaultName, Direction::Input),
                    "Name for the " + Strings::toString(i) + "th dimension. Leave blank for NONE.");
    auto atMost3 = std::make_shared<ArrayLengthValidator<double>>(0, 3);
    std::vector<double> temp;
    declareProperty(std::make_unique<ArrayProperty<double>>(propBinning, temp, atMost3),
                    "Binning for the " + Strings::toString(i) + "th dimension.\n" +
                        "- Leave blank for complete integration\n" +
                        "- One value is interpreted as step\n"
                        "- Two values are interpreted integration interval\n" +
                        "- Three values are interpreted as min, step, max");
    setPropertyGroup(propName, "Binning");
    setPropertyGroup(propBinning, "Binning");
  }

  // symmetry operations
  declareProperty(std::make_unique<PropertyWithValue<std::string>>("SymmetryOperations", "", Direction::Input),
                  "If specified the symmetry will be applied, "
                  "can be space group name, point group name, or list "
                  "individual symmetries.");

  // temporary workspaces
  declareProperty(std::make_unique<WorkspaceProperty<IMDHistoWorkspace>>("TemporaryDataWorkspace", "", Direction::Input,
                                                                         PropertyMode::Optional),
                  "An (optional) input MDHistoWorkspace used to accumulate data from "
                  "multiple MDEventWorkspaces. If unspecified a blank "
                  "MDHistoWorkspace will be created.");
  declareProperty(std::make_unique<WorkspaceProperty<IMDHistoWorkspace>>("TemporaryNormalizationWorkspace", "",
                                                                         Direction::Input, PropertyMode::Optional),
                  "An (optional) input MDHistoWorkspace used to accumulate normalization "
                  "from multiple MDEventWorkspaces. If unspecified a blank "
                  "MDHistoWorkspace will be created.");

  // temporary background workspace
  declareProperty(std::make_unique<WorkspaceProperty<IMDHistoWorkspace>>("TemporaryBackgroundDataWorkspace", "",
                                                                         Direction::Input, PropertyMode::Optional),
                  "An (optional) input MDHistoWorkspace used to accumulate background from "
                  "multiple background MDEventWorkspaces. If unspecified but "
                  "BackgroundWorkspace is specified, a blank "
                  "MDHistoWorkspace will be created.");
  declareProperty(std::make_unique<WorkspaceProperty<IMDHistoWorkspace>>("TemporaryBackgroundNormalizationWorkspace",
                                                                         "", Direction::Input, PropertyMode::Optional),
                  "An (optional) input MDHistoWorkspace used to accumulate background normalization "
                  "from multiple background MDEventWorkspaces. If unspecified but "
                  "BackgroundWorkspace is specified, a blank "
                  "MDHistoWorkspace will be created.");

  setPropertyGroup("TemporaryDataWorkspace", "Temporary workspaces");
  setPropertyGroup("TemporaryNormalizationWorkspace", "Temporary workspaces");
  setPropertyGroup("TemporaryBackgroundDataWorkspace", "Temporary workspaces");
  setPropertyGroup("TemporaryBackgroundNormalizationWorkspace", "Temporary workspaces");

  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>("OutputWorkspace", "", Kernel::Direction::Output),
                  "A name for the normalized output MDHistoWorkspace.");
  declareProperty(
      std::make_unique<WorkspaceProperty<API::Workspace>>("OutputDataWorkspace", "", Kernel::Direction::Output),
      "A name for the output data MDHistoWorkspace.");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputNormalizationWorkspace", "", Direction::Output),
                  "A name for the output normalization MDHistoWorkspace.");
  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>(
                      "OutputBackgroundDataWorkspace", "", Kernel::Direction::Output, PropertyMode::Optional),
                  "A name for the optional output background data MDHistoWorkspace.");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputBackgroundNormalizationWorkspace", "",
                                                                 Direction::Output, PropertyMode::Optional),
                  "A name for the optional output background normalization MDHistoWorkspace.");
}

//----------------------------------------------------------------------------------------------
/// Validate the input workspace @see Algorithm::validateInputs
std::map<std::string, std::string> MDNorm::validateInputs() {
  std::map<std::string, std::string> errorMessage;

  // Check for input workspace frame
  Mantid::API::IMDEventWorkspace_sptr inputWS = this->getProperty("InputWorkspace");
  if (inputWS->getNumDims() < 3) {
    errorMessage.emplace("InputWorkspace", "The input workspace must be at least 3D");
  } else {
    for (size_t i = 0; i < 3; i++) {
      if (inputWS->getDimension(i)->getMDFrame().name() != Mantid::Geometry::QSample::QSampleName) {
        errorMessage.emplace("InputWorkspace", "The input workspace must be in Q_sample");
      }
    }
  }

  // Optional background input IMDE
  Mantid::API::IMDEventWorkspace_sptr bkgdWS = this->getProperty("BackgroundWorkspace");
  if (bkgdWS) {
    if (bkgdWS->getNumDims() < 3) {
      // must have at least 3 dimensions
      errorMessage.emplace("BackgroundWorkspace", "The input background workspace must be at least 3D");
    } else {
      // Check first 3 dimension for Q lab,
      for (size_t i = 0; i < 3; i++) {
        if (bkgdWS->getDimension(i)->getMDFrame().name() != Mantid::Geometry::QLab::QLabName) {
          errorMessage.emplace("BackgroundWorkspace", "The input backgound workspace must be in Q_lab");
        }
      }

      // Check 4th dimension if input workspace is elastic
      if (inputWS->getNumDims() > 3) {
        if (bkgdWS->getNumDims() <= 3) {
          errorMessage.emplace("BackgroundWorkspace", "The input background workspace must have at 4 dimensions when "
                                                      "input workspace has more than 4 dimensions (inelastic case).");
        } else if (bkgdWS->getDimension(3)->getName() != inputWS->getDimension(3)->getName()) {
          errorMessage.emplace("BackgroundWorkspace", "The input background workspace 4th dimension must be DeltaE "
                                                      "for inelastic case.");
        }
      }
    }
  }

  // Check if the vanadium is available for diffraction
  bool diffraction = true;
  if ((inputWS->getNumDims() > 3) && (inputWS->getDimension(3)->getName() == "DeltaE")) {
    diffraction = false;
  }
  if (diffraction) {
    API::MatrixWorkspace_const_sptr solidAngleWS = getProperty("SolidAngleWorkspace");
    API::MatrixWorkspace_const_sptr fluxWS = getProperty("FluxWorkspace");
    if (solidAngleWS == nullptr) {
      errorMessage.emplace("SolidAngleWorkspace", "SolidAngleWorkspace is required for diffraction");
    }
    if (fluxWS == nullptr) {
      errorMessage.emplace("FluxWorkspace", "FluxWorkspace is required for diffraction");
    }
  }
  // Check for property MDNorm_low and MDNorm_high
  size_t nExperimentInfos = inputWS->getNumExperimentInfo();
  if (nExperimentInfos == 0) {
    errorMessage.emplace("InputWorkspace", "There must be at least one experiment info");
  } else {
    for (size_t iExpInfo = 0; iExpInfo < nExperimentInfos; iExpInfo++) {
      auto &currentExptInfo = *(inputWS->getExperimentInfo(static_cast<uint16_t>(iExpInfo)));
      if (!currentExptInfo.run().hasProperty("MDNorm_low")) {
        errorMessage.emplace("InputWorkspace", "Missing MDNorm_low log. Please "
                                               "use CropWorkspaceForMDNorm "
                                               "before converting to MD");
      }
      if (!currentExptInfo.run().hasProperty("MDNorm_high")) {
        errorMessage.emplace("InputWorkspace", "Missing MDNorm_high log. Please use "
                                               "CropWorkspaceForMDNorm before converting to MD");
      }
    }
  }
  // check projections and UB
  if (getProperty("RLU")) {
    DblMatrix W = DblMatrix(3, 3);
    std::vector<double> Q0Basis = getProperty("QDimension0");
    std::vector<double> Q1Basis = getProperty("QDimension1");
    std::vector<double> Q2Basis = getProperty("QDimension2");
    W.setColumn(0, Q0Basis);
    W.setColumn(1, Q1Basis);
    W.setColumn(2, Q2Basis);
    if (fabs(W.determinant()) < 1e-5) {
      errorMessage.emplace("QDimension0", "The projection dimensions are coplanar or zero");
      errorMessage.emplace("QDimension1", "The projection dimensions are coplanar or zero");
      errorMessage.emplace("QDimension2", "The projection dimensions are coplanar or zero");
    }
    if (!inputWS->getExperimentInfo(0)->sample().hasOrientedLattice()) {
      errorMessage.emplace("InputWorkspace", "There is no oriented lattice "
                                             "associated with the input workspace. "
                                             "Use SetUB algorithm");
    }
  }
  // check dimension names
  std::vector<std::string> originalDimensionNames;
  for (size_t i = 3; i < inputWS->getNumDims(); i++) {
    originalDimensionNames.emplace_back(inputWS->getDimension(i)->getName());
  }
  originalDimensionNames.emplace_back("QDimension0");
  originalDimensionNames.emplace_back("QDimension1");
  originalDimensionNames.emplace_back("QDimension2");
  std::vector<std::string> selectedDimensions;
  for (std::size_t i = 0; i < 6; i++) {
    std::string propName = "Dimension" + Strings::toString(i) + "Name";
    std::string dimName = getProperty(propName);
    std::string binningName = "Dimension" + Strings::toString(i) + "Binning";
    std::vector<double> binning = getProperty(binningName);
    if (!dimName.empty()) {
      auto it = std::find(originalDimensionNames.begin(), originalDimensionNames.end(), dimName);
      if (it == originalDimensionNames.end()) {
        errorMessage.emplace(propName, "Name '" + dimName +
                                           "' is not one of the "
                                           "original workspace names or a directional dimension");
      } else {
        // make sure dimension is unique
        auto itSel = std::find(selectedDimensions.begin(), selectedDimensions.end(), dimName);
        if (itSel == selectedDimensions.end()) {
          selectedDimensions.emplace_back(dimName);
        } else {
          errorMessage.emplace(propName, "Name '" + dimName + "' was already selected");
        }
      }
    } else {
      if (!binning.empty()) {
        errorMessage.emplace(binningName, "There should be no binning if the dimension name is empty");
      }
    }
  }
  // since Q dimensions can be non - orthogonal, all must be present
  if ((std::find(selectedDimensions.begin(), selectedDimensions.end(), "QDimension0") == selectedDimensions.end()) ||
      (std::find(selectedDimensions.begin(), selectedDimensions.end(), "QDimension1") == selectedDimensions.end()) ||
      (std::find(selectedDimensions.begin(), selectedDimensions.end(), "QDimension2") == selectedDimensions.end())) {
    for (std::size_t i = 0; i < 6; i++) {
      std::string propName = "Dimension" + Strings::toString(i) + "Name";
      errorMessage.emplace(propName, "All of QDimension0, QDimension1, QDimension2 must be present");
    }
  }
  // symmetry operations
  std::string symOps = this->getProperty("SymmetryOperations");
  if (!symOps.empty()) {
    bool isSpaceGroup = Geometry::SpaceGroupFactory::Instance().isSubscribed(symOps);
    bool isPointGroup = Geometry::PointGroupFactory::Instance().isSubscribed(symOps);
    if (!isSpaceGroup && !isPointGroup) {
      try {
        Geometry::SymmetryOperationFactory::Instance().createSymOps(symOps);
      } catch (const Mantid::Kernel::Exception::ParseError &) {
        errorMessage.emplace("SymmetryOperations", "The input is not a space group, a point group, "
                                                   "or a list of symmetry operations");
      }
    }
  }
  // validate accumulation workspaces, if provided
  std::shared_ptr<IMDHistoWorkspace> tempNormWS = this->getProperty("TemporaryNormalizationWorkspace");
  Mantid::API::IMDHistoWorkspace_sptr tempDataWS = this->getProperty("TemporaryDataWorkspace");

  // check that either both or neuther accumulation workspaces are provied
  if ((tempNormWS && !tempDataWS) || (!tempNormWS && tempDataWS)) {
    errorMessage.emplace("TemporaryDataWorkspace", "Must provide either no accumulation workspaces or,"
                                                   "both TemporaryNormalizationWorkspaces and TemporaryDataWorkspace");
  }
  // check that both accumulation workspaces are on the same grid
  if (tempNormWS && tempDataWS) {
    size_t numNormDims = tempNormWS->getNumDims();
    size_t numDataDims = tempDataWS->getNumDims();
    if (numNormDims == numDataDims) {
      for (size_t i = 0; i < numNormDims; i++) {
        const auto dim1 = tempNormWS->getDimension(i);
        const auto dim2 = tempDataWS->getDimension(i);
        if ((dim1->getMinimum() != dim2->getMinimum()) || (dim1->getMaximum() != dim2->getMaximum()) ||
            (dim1->getNBins() != dim2->getNBins()) || (dim1->getName() != dim2->getName())) {
          errorMessage.emplace("TemporaryDataWorkspace", "Binning for TemporaryNormalizationWorkspaces "
                                                         "and TemporaryDataWorkspace must be the same.");
          break;
        }
      }
    } else { // accumulation workspaces have different number of dimensions
      errorMessage.emplace("TemporaryDataWorkspace", "TemporaryNormalizationWorkspace and TemporaryDataWorkspace "
                                                     "do not have the same number of dimensions");
    }
  }

  // validate accumulated background workspaces
  Mantid::API::IMDHistoWorkspace_sptr tempBkgdDataWS = this->getProperty("TemporaryBackgroundDataWorkspace");
  Mantid::API::IMDHistoWorkspace_sptr tempBkgdNormWS = this->getProperty("TemporaryBackgroundNormalizationWorkspace");
  // check existing criteria: Background, TempBackgroundData and
  // TempBackgroundNormalization must be specified
  if (tempBkgdDataWS && (!bkgdWS || !tempDataWS || !tempBkgdNormWS)) {
    errorMessage.emplace("TemporaryBackgroundDataWorkspace", "TemporaryBackgroundDataWorkspace is specified but at "
                                                             "least one of these is not.");
  } else if (tempBkgdNormWS && (!bkgdWS || !tempNormWS || !tempBkgdDataWS)) {
    errorMessage.emplace("TemporaryBackgroundNormalizationWorkspace", "TemporaryBackgroundNormalizationWorkspace is "
                                                                      "specified but at least one of these is not.");
  } else if (bkgdWS && tempDataWS && !tempBkgdDataWS) {
    errorMessage.emplace("TemporaryDataWorkspace",
                         "With Background is specifed and TemporaryDataWorkspace is specifed, "
                         "TemporaryBackgroundDataWorkspace must be specified.");
  } else if (tempBkgdDataWS && tempNormWS) {
    // check when they both exist
    size_t numBkgdDataDims = tempBkgdDataWS->getNumDims();
    size_t numBkgdNormDims = tempBkgdNormWS->getNumDims();
    size_t numDataDims = tempDataWS->getNumDims();
    if (numBkgdDataDims == numBkgdNormDims && numBkgdDataDims == numDataDims) {
      // On each dimension, compare min, max, NBins and name
      for (size_t idim = 0; idim < numBkgdDataDims; ++idim) {
        const auto dimB = tempBkgdDataWS->getDimension(idim);
        const auto dimN = tempBkgdNormWS->getDimension(idim);
        const auto dimD = tempDataWS->getDimension(idim);
        if ((dimB->getMinimum() != dimN->getMinimum()) || (dimB->getMinimum() != dimD->getMinimum()) ||
            (dimB->getMaximum() != dimN->getMaximum()) || (dimB->getMaximum() != dimD->getMaximum()) ||
            (dimB->getNBins() != dimN->getNBins()) || (dimB->getNBins() != dimD->getNBins()) ||
            (dimB->getName() != dimN->getName()) || (dimB->getName() != dimD->getName())) {
          errorMessage.emplace("TemporaryBackgroundDataWorkspace",
                               "TemporaryBackgroundDataWorkspace, "
                               "TemporaryBackgroundNormalizationWorkspace and "
                               "TemporaryDataWorkspace "
                               "must have same minimum, maximum, number of bins and name.");
          break;
        }
      }
    } else {
      errorMessage.emplace("TemporaryBackgroundDataWorkspace", "TemporaryBackgroundDataWorkspace, "
                                                               "TemporaryBackgroundNormalizationWorkspace and "
                                                               "TemporaryDataWorkspace must have same dimensions");
    }
  }

  return errorMessage;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void MDNorm::exec() {
  convention = Kernel::ConfigService::Instance().getString("Q.convention");
  // symmetry operations
  std::string symOps = this->getProperty("SymmetryOperations");
  std::vector<Geometry::SymmetryOperation> symmetryOps;
  if (symOps.empty()) {
    symOps = "x,y,z";
  }
  if (Geometry::SpaceGroupFactory::Instance().isSubscribed(symOps)) {
    auto spaceGroup = Geometry::SpaceGroupFactory::Instance().createSpaceGroup(symOps);
    auto pointGroup = spaceGroup->getPointGroup();
    symmetryOps = pointGroup->getSymmetryOperations();
  } else if (Geometry::PointGroupFactory::Instance().isSubscribed(symOps)) {
    auto pointGroup = Geometry::PointGroupFactory::Instance().createPointGroup(symOps);
    symmetryOps = pointGroup->getSymmetryOperations();
  } else {
    symmetryOps = Geometry::SymmetryOperationFactory::Instance().createSymOps(symOps);
  }
  g_log.debug() << "Symmetry operations\n";
  for (auto so : symmetryOps) {
    g_log.debug() << so.identifier() << "\n";
  }
  m_numSymmOps = symmetryOps.size();

  m_isRLU = getProperty("RLU");
  // get the workspaces
  m_inputWS = this->getProperty("InputWorkspace");
  const auto &exptInfoZero = *(m_inputWS->getExperimentInfo(0));
  auto source = exptInfoZero.getInstrument()->getSource();
  auto sample = exptInfoZero.getInstrument()->getSample();
  if (source == nullptr || sample == nullptr) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Instrument not sufficiently defined: failed to get source and/or "
        "sample");
  }
  m_samplePos = sample->getPos();
  m_beamDir = normalize(m_samplePos - source->getPos());
  if ((m_inputWS->getNumDims() > 3) && (m_inputWS->getDimension(3)->getName() == "DeltaE")) {
    // DeltaE in input MDE: it cannot be diffraction!
    m_diffraction = false;
    if (exptInfoZero.run().hasProperty("Ei")) {
      Kernel::Property *eiprop = exptInfoZero.run().getProperty("Ei");
      m_Ei = boost::lexical_cast<double>(eiprop->value());
      if (m_Ei <= 0) {
        throw std::invalid_argument("Ei stored in the workspace is not positive");
      }
    } else {
      throw std::invalid_argument("Could not find Ei value in the workspace.");
    }
  }

  // Calculate (BinMD) input sample MDE to MDH and create noramlization MDH from
  // it
  auto outputDataWS = binInputWS(symmetryOps);
  createNormalizationWS(*outputDataWS);
  this->setProperty("OutputNormalizationWorkspace", m_normWS);
  this->setProperty("OutputDataWorkspace", outputDataWS);

  // Background
  m_backgroundWS = this->getProperty("BackgroundWorkspace");
  DataObjects::MDHistoWorkspace_sptr outputBackgroundDataWS(nullptr);
  // Outputs for background related
  if (m_backgroundWS) {
    outputBackgroundDataWS = binBackgroundWS(symmetryOps);
    createBackgroundNormalizationWS(*outputBackgroundDataWS);
    this->setProperty("OutputBackgroundNormalizationWorkspace", m_bkgdNormWS);
    this->setProperty("OutputBackgroundDataWorkspace", outputBackgroundDataWS);
  }

  m_numExptInfos = outputDataWS->getNumExperimentInfo();
  // loop over all experiment infos
  for (uint16_t expInfoIndex = 0; expInfoIndex < m_numExptInfos; expInfoIndex++) {
    // Check for other dimensions if we could measure anything in the original
    // data
    bool skipNormalization = false;
    const std::vector<coord_t> otherValues = getValuesFromOtherDimensions(skipNormalization, expInfoIndex);

    cacheDimensionXValues();

    if (!skipNormalization) {
      size_t symmOpsIndex = 0;
      for (const auto &so : symmetryOps) {
        calculateNormalization(otherValues, so, expInfoIndex, symmOpsIndex);
        symmOpsIndex++;
      }

    } else {
      g_log.warning("Binning limits are outside the limits of the MDWorkspace. "
                    "Not applying normalization.");
    }
    // if more than one experiment info, keep accumulating
    m_accumulate = true;
  }

  API::IMDWorkspace_sptr out(nullptr);

  if (m_backgroundWS) {
    // Normalize binned (BinMD) sample workspace with background
    out = divideMD(outputDataWS, m_normWS, getPropertyValue("OutputWorkspace"), 0.97, 0.98);

    // Normalize background
    const std::string normedBkgdWSName("_normedBkgd");
    API::IMDWorkspace_sptr outbkgd = divideMD(outputBackgroundDataWS, m_bkgdNormWS, normedBkgdWSName, 0.98, 0.99);

    // Clean workspace
    auto minusMD = createChildAlgorithm("MinusMD", 0.99, 1.00);
    // set up
    minusMD->setProperty("LHSWorkspace", out);
    minusMD->setProperty("RHSWorkspace", outbkgd);
    minusMD->setPropertyValue("OutputWorkspace", getPropertyValue("OutputWorkspace"));
    // run and return
    minusMD->executeAsChildAlg();
    out = minusMD->getProperty("OutputWorkspace");

  } else {
    // Normalize binned (BinMD) sample workspace without background
    out = divideMD(outputDataWS, m_normWS, getPropertyValue("OutputWorkspace"), 0.97, 1.);
  }

  // Set output workspace
  this->setProperty("OutputWorkspace", out);
}

inline API::IMDWorkspace_sptr MDNorm::divideMD(const API::IMDHistoWorkspace_sptr &lhs,
                                               const API::IMDHistoWorkspace_sptr &rhs, const std::string &outputwsname,
                                               const double &startProgress, const double &endProgress) {
  auto divideMD = createChildAlgorithm("DivideMD", startProgress, endProgress);
  divideMD->setProperty("LHSWorkspace", lhs);
  divideMD->setProperty("RHSWorkspace", rhs);
  divideMD->setPropertyValue("OutputWorkspace", outputwsname);
  divideMD->executeAsChildAlg();
  // API::IMDWorkspace_sptr
  API::IMDWorkspace_sptr out = divideMD->getProperty("OutputWorkspace");

  return out;
}

/**
 * Get the dimension name when not using reciprocal lattice units.
 * @param i - axis number to return axis name for.  Can be 0, 1, or 2.
 * @return string containing the name
 */
std::string MDNorm::QDimensionNameQSample(int i) {
  if (i == 0)
    return std::string("Q_sample_x");
  else if (i == 1)
    return std::string("Q_sample_y");
  else if (i == 2)
    return std::string("Q_sample_z");
  else
    throw std::invalid_argument("Index must be 0, 1, or 2 for QDimensionNameQSample");
}
/**
 * Get the dimension name when using reciprocal lattice units.
 * @param projection - a vector with 3 elements, containing a
 *   description of the projection ("1,-1,0" for "[H,-H,0]")
 * @return string containing the name
 */
std::string MDNorm::QDimensionName(std::vector<double> projection) {
  std::vector<double>::iterator result;
  result = std::max_element(projection.begin(), projection.end(), abs_compare);
  std::vector<char> symbol{'H', 'K', 'L'};
  char character = symbol[std::distance(projection.begin(), result)];
  std::stringstream name;
  name << "[";
  for (size_t i = 0; i < 3; i++) {
    if (projection[i] == 0) {
      name << "0";
    } else if (projection[i] == 1) {
      name << character;
    } else if (projection[i] == -1) {
      name << "-" << character;
    } else {
      name << std::defaultfloat << std::setprecision(3) << projection[i] << character;
    }
    if (i != 2) {
      name << ",";
    }
  }
  name << "]";
  return name.str();
}

/**
 * Calculate binning parameters
 * @return map of parameters to be passed to BinMD (non axis-aligned)
 */
std::map<std::string, std::string> MDNorm::getBinParameters() {
  std::map<std::string, std::string> parameters;
  std::stringstream extents;
  std::stringstream bins;
  std::vector<std::string> originalDimensionNames;
  originalDimensionNames.emplace_back("QDimension0");
  originalDimensionNames.emplace_back("QDimension1");
  originalDimensionNames.emplace_back("QDimension2");
  for (size_t i = 3; i < m_inputWS->getNumDims(); i++) {
    originalDimensionNames.emplace_back(m_inputWS->getDimension(i)->getName());
  }

  if (m_isRLU) {
    m_Q0Basis = getProperty("QDimension0");
    m_Q1Basis = getProperty("QDimension1");
    m_Q2Basis = getProperty("QDimension2");
    m_UB = m_inputWS->getExperimentInfo(0)->sample().getOrientedLattice().getUB() * 2 * M_PI;
  }

  std::vector<double> W(m_Q0Basis);
  W.insert(W.end(), m_Q1Basis.begin(), m_Q1Basis.end());
  W.insert(W.end(), m_Q2Basis.begin(), m_Q2Basis.end());
  m_W = DblMatrix(W);
  m_W.Transpose();

  // Find maximum Q
  auto &exptInfo0 = *(m_inputWS->getExperimentInfo(static_cast<uint16_t>(0)));
  auto upperLimitsVector =
      (*(dynamic_cast<Kernel::PropertyWithValue<std::vector<double>> *>(exptInfo0.getLog("MDNorm_high"))))();
  double maxQ;
  if (m_diffraction) {
    maxQ = 2. * (*std::max_element(upperLimitsVector.begin(), upperLimitsVector.end()));
  } else {
    double Ei;
    double maxDE = *std::max_element(upperLimitsVector.begin(), upperLimitsVector.end());
    auto loweLimitsVector =
        (*(dynamic_cast<Kernel::PropertyWithValue<std::vector<double>> *>(exptInfo0.getLog("MDNorm_low"))))();
    double minDE = *std::min_element(loweLimitsVector.begin(), loweLimitsVector.end());
    if (exptInfo0.run().hasProperty("Ei")) {
      Kernel::Property *eiprop = exptInfo0.run().getProperty("Ei");
      Ei = boost::lexical_cast<double>(eiprop->value());
      if (Ei <= 0) {
        throw std::invalid_argument("Ei stored in the workspace is not positive");
      }
    } else {
      throw std::invalid_argument("Could not find Ei value in the workspace.");
    }
    const double energyToK = 8.0 * M_PI * M_PI * PhysicalConstants::NeutronMass * PhysicalConstants::meV * 1e-20 /
                             (PhysicalConstants::h * PhysicalConstants::h);
    double ki = std::sqrt(energyToK * Ei);
    double kfmin = std::sqrt(energyToK * (Ei - minDE));
    double kfmax = std::sqrt(energyToK * (Ei - maxDE));

    maxQ = ki + std::max(kfmin, kfmax);
  }
  size_t basisVectorIndex = 0;
  std::vector<coord_t> transformation;
  for (std::size_t i = 0; i < 6; i++) {
    std::string propName = "Dimension" + Strings::toString(i) + "Name";
    std::string binningName = "Dimension" + Strings::toString(i) + "Binning";
    std::string dimName = getProperty(propName);
    std::vector<double> binning = getProperty(binningName);
    std::string bv = "BasisVector";
    if (!dimName.empty()) {
      std::string property = bv + Strings::toString(basisVectorIndex);
      std::stringstream propertyValue;
      propertyValue << dimName;
      // get the index in the original workspace
      auto dimIndex = std::distance(originalDimensionNames.begin(),
                                    std::find(originalDimensionNames.begin(), originalDimensionNames.end(), dimName));
      auto dimension = m_inputWS->getDimension(dimIndex);
      propertyValue << "," << dimension->getMDUnits().getUnitLabel().ascii();
      for (size_t j = 0; j < originalDimensionNames.size(); j++) {
        if (j == static_cast<size_t>(dimIndex)) {
          propertyValue << ",1";
          transformation.emplace_back(1.f);
        } else {
          propertyValue << ",0";
          transformation.emplace_back(0.f);
        }
      }
      parameters.emplace(property, propertyValue.str());
      // get the extents an number of bins
      coord_t dimMax = dimension->getMaximum();
      coord_t dimMin = dimension->getMinimum();
      if (m_isRLU) {
        Mantid::Geometry::OrientedLattice ol;
        ol.setUB(m_UB * m_W); // note that this is already multiplied by 2Pi
        if (dimIndex == 0) {
          dimMax = static_cast<coord_t>(ol.a() * maxQ);
          dimMin = -dimMax;
        } else if (dimIndex == 1) {
          dimMax = static_cast<coord_t>(ol.b() * maxQ);
          dimMin = -dimMax;
        } else if (dimIndex == 2) {
          dimMax = static_cast<coord_t>(ol.c() * maxQ);
          dimMin = -dimMax;
        }
      }
      if (binning.size() == 0) {
        // only one bin, integrating from min to max
        extents << dimMin << "," << dimMax << ",";
        bins << 1 << ",";
      } else if (binning.size() == 2) {
        // only one bin, integrating from min to max
        extents << binning[0] << "," << binning[1] << ",";
        bins << 1 << ",";
      } else if (binning.size() == 1) {
        auto step = binning[0];
        double nsteps = (dimMax - dimMin) / step;
        if (nsteps + 1 - std::ceil(nsteps) >= 1e-4) {
          nsteps = std::ceil(nsteps);
        } else {
          nsteps = std::floor(nsteps);
        }
        bins << static_cast<int>(nsteps) << ",";
        extents << dimMin << "," << dimMin + nsteps * step << ",";
      } else if (binning.size() == 3) {
        dimMin = static_cast<coord_t>(binning[0]);
        auto step = binning[1];
        dimMax = static_cast<coord_t>(binning[2]);
        double nsteps = (dimMax - dimMin) / step;
        if (nsteps + 1 - std::ceil(nsteps) >= 1e-4) {
          nsteps = std::ceil(nsteps);
        } else {
          nsteps = std::floor(nsteps);
        }
        bins << static_cast<int>(nsteps) << ",";
        extents << dimMin << "," << dimMin + nsteps * step << ",";
      }
      basisVectorIndex++;
    }
  }
  parameters.emplace("OutputExtents", extents.str());
  parameters.emplace("OutputBins", bins.str());
  m_transformation = Mantid::Kernel::Matrix<coord_t>(
      transformation, static_cast<size_t>((transformation.size()) / m_inputWS->getNumDims()), m_inputWS->getNumDims());
  return parameters;
}

/**
 * Create & cached the normalization workspace
 * @param dataWS The binned workspace that will be used for the data
 */
void MDNorm::createNormalizationWS(const DataObjects::MDHistoWorkspace &dataWS) {
  // Copy the MDHisto workspace, and change signals and errors to 0.
  std::shared_ptr<IMDHistoWorkspace> tmp = this->getProperty("TemporaryNormalizationWorkspace");
  m_normWS = std::dynamic_pointer_cast<MDHistoWorkspace>(tmp);
  if (!m_normWS) {
    m_normWS = dataWS.clone();
    m_normWS->setTo(0., 0., 0.);
  } else {
    // Temp is given.  Accumulation mode is on
    m_accumulate = true;
  }
}

void MDNorm::createBackgroundNormalizationWS(const DataObjects::MDHistoWorkspace &bkgdDataWS) {

  // requiring background workspace is specified
  if (!m_backgroundWS) {
    return;
  }

  // Copy the MDHisto workspace, and change signals and errors to 0.
  std::shared_ptr<IMDHistoWorkspace> tmp = this->getProperty("TemporaryBackgroundNormalizationWorkspace");
  m_bkgdNormWS = std::dynamic_pointer_cast<MDHistoWorkspace>(tmp);
  if (!m_bkgdNormWS) {
    m_bkgdNormWS = bkgdDataWS.clone();
    m_bkgdNormWS->setTo(0., 0., 0.);
  }
}

/**
 * Validates the TemporaryDataWorkspace has the same binning
 * as the input binning parameters
 * @param parameters :: map of binning parameters
 * @param tempDataWS :: the workspace weare using to aggregate from
 * @return :: bool - true means the binning is correct to aggreagete using
 * tempDataWS
 */
void MDNorm::validateBinningForTemporaryDataWorkspace(const std::map<std::string, std::string> &parameters,
                                                      const Mantid::API::IMDHistoWorkspace_sptr &tempDataWS) {

  // parse the paramters map and get extents from tempDataWS
  const std::string numBinsStr = parameters.at("OutputBins");
  const std::string extentsStr = parameters.at("OutputExtents");
  const std::vector<size_t> numBins = VectorHelper::splitStringIntoVector<size_t>(numBinsStr);
  const std::vector<double> extents = VectorHelper::splitStringIntoVector<double>(extentsStr);

  // make sure the number of dimensions is the same for both workspaces
  size_t numDimsTemp = tempDataWS->getNumDims();
  if ((numBins.size() != numDimsTemp) || (extents.size() != numDimsTemp * 2)) {
    std::stringstream errorMessage;
    errorMessage << "The number of dimensions in the output and ";
    errorMessage << "TemporaryDataWorkspace are not the same.";
    throw(std::invalid_argument(errorMessage.str()));
  }

  // compare the extents and number of bins
  for (size_t i = 0; i < numDimsTemp; i++) {
    auto ax = tempDataWS->getDimension(i);
    if (numBins[i] != ax->getNBins()) {
      std::stringstream errorMessage;
      errorMessage << "The number of bins output and number of bins in ";
      errorMessage << "TemporaryDataWorkspace are not the same along ";
      errorMessage << "dimension " << i;
      throw(std::invalid_argument(errorMessage.str()));
    }
    if (std::abs(extents[2 * i] - ax->getMinimum()) > 1.e-5) {
      std::stringstream errorMessage;
      errorMessage << "The minimum binning value for the output and ";
      errorMessage << "TemporaryDataWorkspace are not the same along ";
      errorMessage << "dimension " << i;
      throw(std::invalid_argument(errorMessage.str()));
    }
    if (std::abs(extents[2 * i + 1] - ax->getMaximum()) > 1.e-5) {
      std::stringstream errorMessage;
      errorMessage << "The maximum binning value for the output and ";
      errorMessage << "TemporaryDataWorkspace are not the same along ";
      errorMessage << "dimension " << i;
      throw(std::invalid_argument(errorMessage.str()));
    }
  }

  // sort out which axes are dimensional and check names
  size_t parametersIndex = 0;
  std::vector<size_t> dimensionIndex(numDimsTemp + 1, 3); // stores h, k, l or Qx, Qy, Qz dimensions
  for (auto const &p : parameters) {
    auto key = p.first;
    auto value = p.second;
    // value starts with QDimension0, then other stuff
    // do not use ==
    if (value.find("QDimension0") != std::string::npos) {
      dimensionIndex[0] = parametersIndex;
      const std::string dimXName = tempDataWS->getDimension(parametersIndex)->getName();
      if (m_isRLU) { // hkl
        if (dimXName != QDimensionName(m_Q0Basis)) {
          std::stringstream errorMessage;
          std::stringstream debugMessage;
          errorMessage << "TemporaryDataWorkspace does not have the  ";
          errorMessage << "correct name for dimension " << parametersIndex;
          debugMessage << "QDimension0 Names: Output will be: " << QDimensionName(m_Q0Basis);
          debugMessage << " TemporaryDataWorkspace: " << dimXName;
          g_log.warning(debugMessage.str());
          throw(std::invalid_argument(errorMessage.str()));
        }
      } else {
        if (dimXName != QDimensionNameQSample(0)) {
          std::stringstream errorMessage;
          std::stringstream debugMessage;
          errorMessage << "TemporaryDataWorkspace does not have the  ";
          errorMessage << "correct name for dimension " << parametersIndex;
          debugMessage << "QDimension0 Names: Output will be: " << QDimensionNameQSample(0);
          debugMessage << " TemporaryDataWorkspace: " << dimXName;
          g_log.warning(debugMessage.str());
          throw(std::invalid_argument(errorMessage.str()));
        }
      }
    } else if (value.find("QDimension1") != std::string::npos) {
      dimensionIndex[1] = parametersIndex;
      const std::string dimYName = tempDataWS->getDimension(parametersIndex)->getName();
      if (m_isRLU) { // hkl
        if (dimYName != QDimensionName(m_Q1Basis)) {
          std::stringstream errorMessage;
          std::stringstream debugMessage;
          errorMessage << "TemporaryDataWorkspace does not have the  ";
          errorMessage << "correct name for dimension " << parametersIndex;
          debugMessage << "QDimension1 Names: Output will be: " << QDimensionName(m_Q1Basis);
          debugMessage << " TemporaryDataWorkspace: " << dimYName;
          g_log.warning(debugMessage.str());
          throw(std::invalid_argument(errorMessage.str()));
        }
      } else {
        if (dimYName != QDimensionNameQSample(1)) {
          std::stringstream errorMessage;
          std::stringstream debugMessage;
          errorMessage << "TemporaryDataWorkspace does not have the  ";
          errorMessage << "correct name for dimension " << parametersIndex;
          debugMessage << "QDimension1 Names: Output will be: " << QDimensionNameQSample(1);
          debugMessage << " TemporaryDataWorkspace: " << dimYName;
          g_log.warning(debugMessage.str());
          throw(std::invalid_argument(errorMessage.str()));
        }
      }
    } else if (value.find("QDimension2") != std::string::npos) {
      dimensionIndex[2] = parametersIndex;
      const std::string dimZName = tempDataWS->getDimension(parametersIndex)->getName();
      if (m_isRLU) { // hkl
        if (dimZName != QDimensionName(m_Q2Basis)) {
          std::stringstream errorMessage;
          std::stringstream debugMessage;
          errorMessage << "TemporaryDataWorkspace does not have the  ";
          errorMessage << "correct name for dimension " << parametersIndex;
          debugMessage << "QDimension2 Names: Output will be: " << QDimensionName(m_Q2Basis);
          debugMessage << " TemporaryDataWorkspace: " << dimZName;
          g_log.warning(debugMessage.str());
          throw(std::invalid_argument(errorMessage.str()));
        }
      } else {
        if (dimZName != QDimensionNameQSample(2)) {
          std::stringstream errorMessage;
          std::stringstream debugMessage;
          errorMessage << "TemporaryDataWorkspace does not have the  ";
          errorMessage << "correct name for dimension " << parametersIndex;
          debugMessage << "QDimension2 Names: Output will be: " << QDimensionNameQSample(2);
          debugMessage << " TemporaryDataWorkspace: " << dimZName;
          g_log.warning(debugMessage.str());
          throw(std::invalid_argument(errorMessage.str()));
        }
      }

    } else if ((key != "OutputBins") && (key != "OutputExtents")) {
      // make sure the names of non-directional dimensions are the same
      const std::string nameData = tempDataWS->getDimension(parametersIndex)->getName();
      if (value.find(nameData) != 0) {
        g_log.error() << "Dimension " << nameData
                      << " from the temporary workspace"
                         " is not one of the binning dimensions, "
                         " or dimensions are in the wrong order."
                      << std::endl;
        throw(std::invalid_argument("Beside the Q dimensions, "
                                    "TemporaryDataWorkspace does not have the "
                                    "same dimension names as OutputWorkspace."));
      }
    }
    parametersIndex++;
  }
  for (auto &idx : dimensionIndex) {
    if (idx > numDimsTemp)
      throw(std::invalid_argument("Cannot find at least one of QDimension0, "
                                  "QDimension1, or QDimension2"));
  }
}

/**
 * Calculate symmetry operation matrix from Symmetry operation
 * @param so :: symmetry operation
 * @return :: matrix
 */
inline DblMatrix MDNorm::buildSymmetryMatrix(const Geometry::SymmetryOperation &so) {
  // calculate dimensions for binning
  DblMatrix soMatrix(3, 3);
  auto v = so.transformHKL(V3D(1, 0, 0));
  soMatrix.setColumn(0, v);
  v = so.transformHKL(V3D(0, 1, 0));
  soMatrix.setColumn(1, v);
  v = so.transformHKL(V3D(0, 0, 1));
  soMatrix.setColumn(2, v);

  return soMatrix;
}

// projection: input/output
// requiring: m_hIdx, m_kIndex, m_lIdx, meidx, m_dEintegrated, m_Q0Basis,
// mQ1Basis,
/**
 * @brief MDNorm::determineBasisVector
 * @param qindex
 * @param value
 * @param Qtransform
 * @param projection
 * @param basisVector
 * @param qDimensionIndices :: output, Q dimension index mapped to input qindex
 */
inline void MDNorm::determineBasisVector(const size_t &qindex, const std::string &value,
                                         const Mantid::Kernel::DblMatrix &Qtransform, std::vector<double> &projection,
                                         std::stringstream &basisVector, std::vector<size_t> &qDimensionIndices) {
  if (value.find("QDimension0") != std::string::npos) {
    m_hIdx = qindex;
    if (!m_isRLU) {
      projection[0] = 1.;
      basisVector << QDimensionNameQSample(0) << ",A^{-1}";
    } else {
      qDimensionIndices.emplace_back(qindex);
      projection[0] = Qtransform[0][0];
      projection[1] = Qtransform[1][0];
      projection[2] = Qtransform[2][0];
      basisVector << QDimensionName(m_Q0Basis) << ", r.l.u.";
    }
  } else if (value.find("QDimension1") != std::string::npos) {
    m_kIdx = qindex;
    if (!m_isRLU) {
      projection[1] = 1.;
      basisVector << QDimensionNameQSample(1) << ",A^{-1}";
    } else {
      qDimensionIndices.emplace_back(qindex);
      projection[0] = Qtransform[0][1];
      projection[1] = Qtransform[1][1];
      projection[2] = Qtransform[2][1];
      basisVector << QDimensionName(m_Q1Basis) << ", r.l.u.";
    }
  } else if (value.find("QDimension2") != std::string::npos) {
    m_lIdx = qindex;
    if (!m_isRLU) {
      projection[2] = 1.;
      basisVector << QDimensionNameQSample(2) << ",A^{-1}";
    } else {
      qDimensionIndices.emplace_back(qindex);
      projection[0] = Qtransform[0][2];
      projection[1] = Qtransform[1][2];
      projection[2] = Qtransform[2][2];
      basisVector << QDimensionName(m_Q2Basis) << ", r.l.u.";
    }
  } else if (value.find("DeltaE") != std::string::npos) {
    m_eIdx = qindex;
    m_dEIntegrated = false;
  }
}

/**
 * Set the output Frame to HKL
 * @param qDimensionIndices
 * @param outputMDHWS :: MDHistoWorkspace to set unit to
 */
inline void MDNorm::setQUnit(const std::vector<size_t> &qDimensionIndices,
                             const Mantid::DataObjects::MDHistoWorkspace_sptr &outputMDHWS) {
  Mantid::Geometry::MDFrameArgument argument(Mantid::Geometry::HKL::HKLName, Mantid::Kernel::Units::Symbol::RLU);
  auto mdFrameFactory = Mantid::Geometry::makeMDFrameFactoryChain();
  Mantid::Geometry::MDFrame_uptr hklFrame = mdFrameFactory->create(argument);
  for (size_t i : qDimensionIndices) {
    auto mdHistoDimension = std::const_pointer_cast<Mantid::Geometry::MDHistoDimension>(
        std::dynamic_pointer_cast<const Mantid::Geometry::MDHistoDimension>(outputMDHWS->getDimension(i)));
    mdHistoDimension->setMDFrame(*hklFrame);
  }
  // add W_matrix
  auto ei = outputMDHWS->getExperimentInfo(0);
  ei->mutableRun().addProperty("W_MATRIX", m_W.getVector(), true);
}

/**
 * Bin(MD) input background MDEventWorkspace
 * @param symmetryOps
 * @return
 */
DataObjects::MDHistoWorkspace_sptr
MDNorm::binBackgroundWS(const std::vector<Geometry::SymmetryOperation> &symmetryOps) {
  // Create output background data histogram MD workspace
  // Either from TemporaryBackgroundDataWorkspace
  // Or from scratch
  Mantid::API::IMDHistoWorkspace_sptr tempBkgdDataWS = this->getProperty("TemporaryBackgroundDataWorkspace");
  Mantid::API::Workspace_sptr outputWS;

  // check that our input matches the temporary workspaces
  std::map<std::string, std::string> parameters = getBinParameters();
  if (tempBkgdDataWS) {
    validateBinningForTemporaryDataWorkspace(parameters, tempBkgdDataWS);
  }

  // For each symmetry operation, do binning MD once
  std::vector<size_t> qDimensionIndices;
  uint16_t numexpinfo = static_cast<uint16_t>(m_inputWS->getNumExperimentInfo());
  if (m_numSymmOps != symmetryOps.size())
    throw std::runtime_error("Symmetry operation number m_umSymops is wrong!");

  for (uint16_t i_expinfo = 0; i_expinfo < numexpinfo; ++i_expinfo) {

    auto rotMatrix = m_inputWS->getExperimentInfo(i_expinfo)->run().getGoniometerMatrix();

    // Reset symmetry operation index
    double soIndex = 0;

    for (auto so : symmetryOps) {
      // Q transformation matrix: From Q_lab to HKL or Q_sample
      // Building symmetric operation matrix
      DblMatrix soMatrix = buildSymmetryMatrix(so);
      // Calculate Q transform matrix
      DblMatrix Qtransform;
      if (m_isRLU) {
        Qtransform = rotMatrix * m_UB * soMatrix * m_W;
      } else {
        Qtransform = rotMatrix * soMatrix * m_W;
      }

      // Set up BinMD for this symmetry opeation
      double progress_fraction = 1. / static_cast<double>(symmetryOps.size() * numexpinfo);
      auto binMD =
          createChildAlgorithm("BinMD", soIndex * 0.3 * progress_fraction, (soIndex + 1) * 0.3 * progress_fraction);

      binMD->setPropertyValue("AxisAligned", "0");
      binMD->setProperty("InputWorkspace", m_backgroundWS);
      binMD->setProperty("TemporaryDataWorkspace", tempBkgdDataWS);
      binMD->setPropertyValue("NormalizeBasisVectors", "0");
      // Set the output Workspace directly to Algorithm's
      // OutputBackgroundDataWorkspace
      binMD->setPropertyValue("OutputWorkspace", getPropertyValue("OutputBackgroundDataWorkspace"));
      // set binning properties
      size_t qindex = 0;
      for (auto const &p : parameters) {
        auto key = p.first;
        auto value = p.second;
        std::stringstream basisVector;
        std::vector<double> projection(m_inputWS->getNumDims(), 0.);
        // value is a string that can start with QDimension0, etc, but contain
        // other stuff. Do not use ==
        determineBasisVector(qindex, value, Qtransform, projection, basisVector, qDimensionIndices);

        if (!basisVector.str().empty()) {
          // reconstruct from calculated basis vector
          for (auto proji : projection) {
            proji = std::abs(proji) > 1e-10 ? proji : 0.0;
            basisVector << "," << proji;
          }
          value = basisVector.str();
        }

        binMD->setPropertyValue(key, value);
        qindex++;
      }
      // execute algorithm
      binMD->executeAsChildAlg();

      // set the temporary workspace to be the output workspace, so it keeps
      // adding different symmetries AND
      // FIXME in future another ExpInfo
      outputWS = binMD->getProperty("OutputWorkspace");
      tempBkgdDataWS = std::dynamic_pointer_cast<MDHistoWorkspace>(outputWS);
      tempBkgdDataWS->clearOriginalWorkspaces();
      tempBkgdDataWS->clearTransforms();
    }
  }
  auto outputMDHWS = std::dynamic_pointer_cast<MDHistoWorkspace>(outputWS);
  // set MDUnits for Q dimensions
  if (m_isRLU) {
    setQUnit(qDimensionIndices, outputMDHWS);
  }

  outputMDHWS->setDisplayNormalization(Mantid::API::NoNormalization);
  return outputMDHWS;
}

/**
 * Runs the BinMD algorithm on the input to provide the output workspace
 * All slicing algorithm properties are passed along
 * @return MDHistoWorkspace as a result of the binning
 */
DataObjects::MDHistoWorkspace_sptr MDNorm::binInputWS(const std::vector<Geometry::SymmetryOperation> &symmetryOps) {
  Mantid::API::IMDHistoWorkspace_sptr tempDataWS = this->getProperty("TemporaryDataWorkspace");
  Mantid::API::Workspace_sptr outputWS;
  std::map<std::string, std::string> parameters = getBinParameters();

  // check that our input matches the temporary workspaces
  if (tempDataWS)
    validateBinningForTemporaryDataWorkspace(parameters, tempDataWS);

  double soIndex = 0;
  std::vector<size_t> qDimensionIndices;
  for (auto so : symmetryOps) {
    // calculate dimensions for binning
    DblMatrix soMatrix = buildSymmetryMatrix(so);
    DblMatrix Qtransform;

    if (m_isRLU) {
      Qtransform = m_UB * soMatrix * m_W;
    } else {
      Qtransform = soMatrix * m_W;
    }

    // bin the data
    double fraction = 1. / static_cast<double>(symmetryOps.size());
    auto binMD = createChildAlgorithm("BinMD", soIndex * 0.3 * fraction, (soIndex + 1) * 0.3 * fraction);
    binMD->setPropertyValue("AxisAligned", "0");
    binMD->setProperty("InputWorkspace", m_inputWS);
    binMD->setProperty("TemporaryDataWorkspace", tempDataWS);
    binMD->setPropertyValue("NormalizeBasisVectors", "0");
    binMD->setPropertyValue("OutputWorkspace", getPropertyValue("OutputDataWorkspace"));
    // set binning properties
    size_t qindex = 0;
    for (auto const &p : parameters) {
      auto key = p.first;
      auto value = p.second;
      std::stringstream basisVector;
      std::vector<double> projection(m_inputWS->getNumDims(), 0.);
      // value is a string that can start with QDimension0, etc, but contain
      // other stuff. Do not use ==
      determineBasisVector(qindex, value, Qtransform, projection, basisVector, qDimensionIndices);

      if (!basisVector.str().empty()) {
        // reconstruct from calculated basis vector
        for (auto proji : projection) {
          proji = std::abs(proji) > 1e-10 ? proji : 0.0;
          basisVector << "," << proji;
        }
        value = basisVector.str();
      }

      binMD->setPropertyValue(key, value);
      qindex++;
    }
    // execute algorithm
    binMD->executeAsChildAlg();
    outputWS = binMD->getProperty("OutputWorkspace");

    // set the temporary workspace to be the output workspace, so it keeps
    // adding different symmetries
    tempDataWS = std::dynamic_pointer_cast<MDHistoWorkspace>(outputWS);
    tempDataWS->clearOriginalWorkspaces();
    tempDataWS->clearTransforms();
    soIndex += 1;
  }

  auto outputMDHWS = std::dynamic_pointer_cast<MDHistoWorkspace>(outputWS);
  // set MDUnits for Q dimensions
  if (m_isRLU) {
    setQUnit(qDimensionIndices, outputMDHWS);
  }

  outputMDHWS->setDisplayNormalization(Mantid::API::NoNormalization);
  return outputMDHWS;
}

/**
 * Retrieve logged values from non-HKL dimensions
 * @param skipNormalization [InOut] Updated to false if any values are outside
 * range measured by input workspace
 * @param expInfoIndex current experiment info index
 * @return A vector of values from other dimensions to be include in normalized
 * MD position calculation
 */
std::vector<coord_t> MDNorm::getValuesFromOtherDimensions(bool &skipNormalization, uint16_t expInfoIndex) const {
  const auto &currentRun = m_inputWS->getExperimentInfo(expInfoIndex)->run();

  std::vector<coord_t> otherDimValues;
  for (size_t i = 3; i < m_inputWS->getNumDims(); i++) {
    const auto dimension = m_inputWS->getDimension(i);
    auto inputDimMin = static_cast<float>(dimension->getMinimum());
    auto inputDimMax = static_cast<float>(dimension->getMaximum());
    coord_t outputDimMin(0), outputDimMax(0);
    bool isIntegrated = true;

    for (size_t j = 0; j < m_transformation.numRows(); j++) {
      if (m_transformation[j][i] == 1) {
        isIntegrated = false;
        outputDimMin = m_normWS->getDimension(j)->getMinimum();
        outputDimMax = m_normWS->getDimension(j)->getMaximum();
      }
    }
    if (dimension->getName() == "DeltaE") {
      if ((inputDimMax < outputDimMin) || (inputDimMin > outputDimMax)) {
        skipNormalization = true;
      }
    } else {
      coord_t value = static_cast<coord_t>(
          currentRun.getLogAsSingleValue(dimension->getName(), Mantid::Kernel::Math::TimeAveragedMean));
      otherDimValues.emplace_back(value);
      if (value < inputDimMin || value > inputDimMax) {
        skipNormalization = true;
      }
      if ((!isIntegrated) && (value < outputDimMin || value > outputDimMax)) {
        skipNormalization = true;
      }
    }
  }
  return otherDimValues;
}

/**
 * Stores the X values from each H,K,L, and optionally DeltaE dimension as
 * member variables
 */
void MDNorm::cacheDimensionXValues() {
  auto &hDim = *m_normWS->getDimension(m_hIdx);
  m_hX.resize(hDim.getNBoundaries());
  for (size_t i = 0; i < m_hX.size(); ++i) {
    m_hX[i] = hDim.getX(i);
  }
  auto &kDim = *m_normWS->getDimension(m_kIdx);
  m_kX.resize(kDim.getNBoundaries());
  for (size_t i = 0; i < m_kX.size(); ++i) {
    m_kX[i] = kDim.getX(i);
  }

  auto &lDim = *m_normWS->getDimension(m_lIdx);
  m_lX.resize(lDim.getNBoundaries());
  for (size_t i = 0; i < m_lX.size(); ++i) {
    m_lX[i] = lDim.getX(i);
  }

  if ((!m_diffraction) && (!m_dEIntegrated)) {
    // NOTE: store k final instead
    auto &eDim = *m_normWS->getDimension(m_eIdx);
    m_eX.resize(eDim.getNBoundaries());
    for (size_t i = 0; i < m_eX.size(); ++i) {
      double temp = m_Ei - eDim.getX(i);
      temp = std::max(temp, 0.);
      m_eX[i] = std::sqrt(energyToK * temp);
    }
  }
}

/**
 * Calculate QTransform = (R * UB * SymmetryOperation * m_W)^-1
 * @param currentExpInfo
 * @param so
 * @return
 */
inline Mantid::Kernel::DblMatrix MDNorm::calQTransform(const ExperimentInfo &currentExpInfo,
                                                       const Geometry::SymmetryOperation &so) {
  // Make it to a method!
  DblMatrix R = currentExpInfo.run().getGoniometerMatrix();
  DblMatrix soMatrix(3, 3);
  auto v = so.transformHKL(V3D(1, 0, 0));
  soMatrix.setColumn(0, v);
  v = so.transformHKL(V3D(0, 1, 0));
  soMatrix.setColumn(1, v);
  v = so.transformHKL(V3D(0, 0, 1));
  soMatrix.setColumn(2, v);
  soMatrix.Invert();
  DblMatrix Qtransform = R * m_UB * soMatrix * m_W;
  Qtransform.Invert();

  return Qtransform;
}

/**
 * Calculate the diffraction MDE's intersection integral of a certain
 * detector/spectru
 * @param intersections: vector of intersections
 * @param xValues: empty vector for X values
 * @param yValues: empty vector of Y values (output)
 * @param integrFlux: integral flux workspace
 * @param wsIdx: workspace index
 */
inline void MDNorm::calcDiffractionIntersectionIntegral(std::vector<std::array<double, 4>> &intersections,
                                                        std::vector<double> &xValues, std::vector<double> &yValues,
                                                        const API::MatrixWorkspace &integrFlux, const size_t &wsIdx) {
  // -- calculate integrals for the intersection --
  // momentum values at intersections
  auto intersectionsBegin = intersections.begin();
  // copy momenta to xValues
  xValues.resize(intersections.size());
  yValues.resize(intersections.size());
  auto x = xValues.begin();
  for (auto it = intersectionsBegin; it != intersections.end(); ++it, ++x) {
    *x = (*it)[3];
  }
  // calculate integrals at momenta from xValues by interpolating between
  // points in spectrum sp
  // of workspace integrFlux. The result is stored in yValues
  calcIntegralsForIntersections(xValues, integrFlux, wsIdx, yValues);
}

/**
 * Calculate the normalization among intersections on a single detector
 * in 1 specific SpectrumInfo/ExperimentInfo
 * @param intersections: intersections
 * @param solid: proton charge
 * @param yValues: diffraction intersection integral and common to sample and background
 * @param vmdDims: MD dimensions
 * @param pos: position from intersecton for memory efficiency
 * @param posNew: transformed positions
 * @param signalArray: (output) normalization
 * @param solidBkgd: background proton charge
 * @param bkgdSignalArray: (output) background normalization
 */
inline void MDNorm::calcSingleDetectorNorm(const std::vector<std::array<double, 4>> &intersections, const double &solid,
                                           std::vector<double> &yValues, const size_t &vmdDims,
                                           std::vector<coord_t> &pos, std::vector<coord_t> &posNew,
                                           std::vector<std::atomic<signal_t>> &signalArray, const double &solidBkgd,
                                           std::vector<std::atomic<signal_t>> &bkgdSignalArray) {

  auto intersectionsBegin = intersections.begin();
  for (auto it = intersectionsBegin + 1; it != intersections.end(); ++it) {

    const auto &curIntSec = *it;
    const auto &prevIntSec = *(it - 1);

    // The full vector isn't used so compute only what is necessary
    // If the difference between 2 adjacent intersection is trivial, no
    // intersection normalization is to be calculated
    double delta, eps;
    if (m_diffraction) {
      // diffraction
      delta = curIntSec[3] - prevIntSec[3];
      eps = 1e-7;
    } else {
      // inelastic
      delta = (curIntSec[3] * curIntSec[3] - prevIntSec[3] * prevIntSec[3]) / energyToK;
      eps = 1e-10;
    }
    if (delta < eps)
      continue; // Assume zero contribution if difference is small

    // Average between two intersections for final position
    // [Task 89] Sample and background have same 'pos[]'
    std::transform(curIntSec.data(), curIntSec.data() + vmdDims, prevIntSec.data(), pos.begin(),
                   [](const double rhs, const double lhs) { return static_cast<coord_t>(0.5 * (rhs + lhs)); });
    signal_t signal;
    signal_t bkgdSignal(0.);
    if (m_diffraction) {
      // Diffraction
      // index of the current intersection
      auto k = static_cast<size_t>(std::distance(intersectionsBegin, it));
      // signal = integral between two consecutive intersections
      signal = (yValues[k] - yValues[k - 1]) * solid;
      if (m_backgroundWS)
        bkgdSignal = (yValues[k] - yValues[k - 1]) * solidBkgd;

    } else {
      // Inelastic
      // transform kf to energy transfer
      pos[3] = static_cast<coord_t>(m_Ei - pos[3] * pos[3] / energyToK);
      // signal = energy distance between two consecutive intersections *solid
      // angle *PC
      signal = solid * delta;
      if (m_backgroundWS)
        bkgdSignal = solidBkgd * delta;
    }

    // Find the coordiate of the new position after transformation
    m_transformation.multiplyPoint(pos, posNew);
    // [Task 89] Is linIndex common to both sample and background?
    size_t linIndex = m_normWS->getLinearIndexAtCoord(posNew.data());
    if (linIndex == size_t(-1))
      continue; // not found

    // Set to output
    // set the calculated signal to
    Mantid::Kernel::AtomicOp(signalArray[linIndex], signal, std::plus<signal_t>());
    // [Task 89]
    if (m_backgroundWS)
      Mantid::Kernel::AtomicOp(bkgdSignalArray[linIndex], bkgdSignal, std::plus<signal_t>());
  }
  return;
}

/**
 * Computed the normalization for the input workspace. Results are stored in
 * m_normWS
 * @param otherValues - values for dimensions other than Q or DeltaE
 * @param so - symmetry operation
 * @param expInfoIndex - current experiment info index
 * @param soIndex - the index of symmetry operation (for progress purposes only)
 */
void MDNorm::calculateNormalization(const std::vector<coord_t> &otherValues, const Geometry::SymmetryOperation &so,
                                    uint16_t expInfoIndex, size_t soIndex) {
  const auto &currentExptInfo = *(m_inputWS->getExperimentInfo(expInfoIndex));
  std::vector<double> lowValues, highValues;
  auto *lowValuesLog = dynamic_cast<VectorDoubleProperty *>(currentExptInfo.getLog("MDNorm_low"));
  lowValues = (*lowValuesLog)();
  auto *highValuesLog = dynamic_cast<VectorDoubleProperty *>(currentExptInfo.getLog("MDNorm_high"));
  highValues = (*highValuesLog)();

  // calculate Q transformation matrix (R * UB * SymmetryOperation * m_W)^-1
  // in order to calculate intersections
  DblMatrix Qtransform = calQTransform(currentExptInfo, so);

  // get proton charges
  const double protonCharge = currentExptInfo.run().getProtonCharge();
  // [Task 89]
  const double protonChargeBkgd =
      (m_backgroundWS != nullptr) ? m_backgroundWS->getExperimentInfo(0)->run().getProtonCharge() : 0;

  const auto &spectrumInfo = currentExptInfo.spectrumInfo();

  // Mappings: solid angle and flux workspaces' detector to ws_index map
  const auto ndets = static_cast<int64_t>(spectrumInfo.size());
  bool haveSA = false;
  API::MatrixWorkspace_const_sptr solidAngleWS = getProperty("SolidAngleWorkspace");
  if (solidAngleWS != nullptr) {
    haveSA = true;
  }
  API::MatrixWorkspace_const_sptr integrFlux = getProperty("FluxWorkspace");
  const detid2index_map solidAngDetToIdx =
      (haveSA) ? solidAngleWS->getDetectorIDToWorkspaceIndexMap() : detid2index_map();
  const detid2index_map fluxDetToIdx =
      (m_diffraction) ? integrFlux->getDetectorIDToWorkspaceIndexMap() : detid2index_map();

  // Define dimension, signal array
  const size_t vmdDims = (m_diffraction) ? 3 : 4;
  std::vector<std::atomic<signal_t>> signalArray(m_normWS->getNPoints());

  size_t numNPoints = (m_backgroundWS) ? m_bkgdNormWS->getNPoints() : 0;
  if (m_backgroundWS && numNPoints != m_normWS->getNPoints()) {
    throw std::runtime_error("N points are different");
  }
  std::vector<std::atomic<signal_t>> bkgdSignalArray(numNPoints);

  std::vector<std::array<double, 4>> intersections;
  std::vector<double> xValues, yValues;
  std::vector<coord_t> pos, posNew;

  // Progress report
  double progStep = 0.7 / static_cast<double>(m_numExptInfos * m_numSymmOps);
  auto progIndex = static_cast<double>(soIndex + expInfoIndex * m_numSymmOps);
  auto prog =
      std::make_unique<API::Progress>(this, 0.3 + progStep * progIndex, 0.3 + progStep * (1. + progIndex), ndets);
  // muliple threading
  bool safe = true;
  if (m_diffraction) {
    safe = Kernel::threadSafe(*integrFlux);
  }

PRAGMA_OMP(parallel for private(intersections, xValues, yValues, pos, posNew) if (safe))
for (int64_t i = 0; i < ndets; i++) {
  PARALLEL_START_INTERUPT_REGION

  // Skip: non-existing detector, monitor and masked detector
  if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMonitor(i) || spectrumInfo.isMasked(i)) {
    continue;
  }

  const auto &detector = spectrumInfo.detector(i);
  double theta = detector.getTwoTheta(m_samplePos, m_beamDir);
  double phi = detector.getPhi();
  // If the dtefctor is a group, this should be the ID of the first detector
  const auto detID = detector.getID();

  // get the flux spectrum number: this is for diffraction only!
  size_t wsIdx = 0;
  if (m_diffraction) {
    auto index = fluxDetToIdx.find(detID);
    if (index != fluxDetToIdx.end()) {
      wsIdx = index->second;
    } else { // masked detector in flux, but not in input workspace
      continue;
    }
  }

  // Intersections for sample and background if present
  this->calculateIntersections(intersections, theta, phi, Qtransform, lowValues[i], highValues[i]);

  // No need to do normalization calculation if there is no intersection
  if (intersections.empty())
    continue;

  // Get solid angle for this contribution
  double solid = protonCharge;
  // [Task 89]
  double bkgdSolid = protonChargeBkgd;
  if (haveSA) {
    double solid_angle_factor = solidAngleWS->y(solidAngDetToIdx.find(detID)->second)[0];
    //  solidAngleWS->y(solidAngDetToIdx.find(detID)->second)[0]
    solid = solid_angle_factor * protonCharge;
    // [Task 89]
    bkgdSolid = solid_angle_factor * protonChargeBkgd;
  }

  if (m_diffraction) {
    // -- calculate integrals for the intersection --
    calcDiffractionIntersectionIntegral(intersections, xValues, yValues, *integrFlux, wsIdx);
  }

  // Compute final position in HKL
  // pre-allocate for efficiency and copy non-hkl dim values into place
  pos.resize(vmdDims + otherValues.size());
  std::copy(otherValues.begin(), otherValues.end(), pos.begin() + vmdDims);

  calcSingleDetectorNorm(intersections, solid, yValues, vmdDims, pos, posNew, signalArray, bkgdSolid,
                         bkgdSignalArray); // [Task 89] ADD solidBkgd, bkgdYValues, bkgdSignalArray

  prog->report();

  PARALLEL_END_INTERUPT_REGION
}
PARALLEL_CHECK_INTERUPT_REGION
if (m_accumulate) {
  std::transform(signalArray.cbegin(), signalArray.cend(), m_normWS->getSignalArray(), m_normWS->mutableSignalArray(),
                 [](const std::atomic<signal_t> &a, const signal_t &b) { return a + b; });
  // [Task 89] Process background
  if (m_backgroundWS)
    std::transform(bkgdSignalArray.cbegin(), bkgdSignalArray.cend(), m_bkgdNormWS->getSignalArray(),
                   m_bkgdNormWS->mutableSignalArray(),
                   [](const std::atomic<signal_t> &a, const signal_t &b) { return a + b; });

} else {
  // First time, init
  std::copy(signalArray.cbegin(), signalArray.cend(), m_normWS->mutableSignalArray());
  // [Task 89]
  if (m_backgroundWS)
    std::copy(bkgdSignalArray.cbegin(), bkgdSignalArray.cend(), m_bkgdNormWS->mutableSignalArray());
}
m_accumulate = true;
}

/**
 * Calculate the points of intersection for the given detector with cuboid
 * surrounding the detector position in HKL
 * @param intersections A list of intersections in HKL space
 * @param theta Polar angle withd detector
 * @param phi Azimuthal angle with detector
 * @param transform Matrix to convert frm Q_lab to HKL (2Pi*R *UB*W*SO)^{-1}
 * @param lowvalue The lowest momentum or energy transfer for the trajectory
 * @param highvalue The highest momentum or energy transfer for the trajectory
 */
void MDNorm::calculateIntersections(std::vector<std::array<double, 4>> &intersections, const double theta,
                                    const double phi, const Kernel::DblMatrix &transform, double lowvalue,
                                    double highvalue) {
  V3D qout(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta)), qin(0., 0., 1);

  qout = transform * qout;
  qin = transform * qin;
  if (convention == "Crystallography") {
    qout *= -1;
    qin *= -1;
  }
  double kfmin, kfmax, kimin, kimax;
  if (m_diffraction) {
    kimin = lowvalue;
    kimax = highvalue;
    kfmin = kimin;
    kfmax = kimax;
  } else {
    kimin = std::sqrt(energyToK * m_Ei);
    kimax = kimin;
    kfmin = std::sqrt(energyToK * (m_Ei - highvalue));
    kfmax = std::sqrt(energyToK * (m_Ei - lowvalue));
  }

  double hStart = qin.X() * kimin - qout.X() * kfmin, hEnd = qin.X() * kimax - qout.X() * kfmax;
  double kStart = qin.Y() * kimin - qout.Y() * kfmin, kEnd = qin.Y() * kimax - qout.Y() * kfmax;
  double lStart = qin.Z() * kimin - qout.Z() * kfmin, lEnd = qin.Z() * kimax - qout.Z() * kfmax;

  double eps = 1e-10;
  auto hNBins = m_hX.size();
  auto kNBins = m_kX.size();
  auto lNBins = m_lX.size();
  auto eNBins = m_eX.size();
  intersections.clear();
  intersections.reserve(hNBins + kNBins + lNBins + eNBins + 2);

  // calculate intersections with planes perpendicular to h
  if (fabs(hStart - hEnd) > eps) {
    double fmom = (kfmax - kfmin) / (hEnd - hStart);
    double fk = (kEnd - kStart) / (hEnd - hStart);
    double fl = (lEnd - lStart) / (hEnd - hStart);
    for (size_t i = 0; i < hNBins; i++) {
      double hi = m_hX[i];
      if (((hStart - hi) * (hEnd - hi) < 0)) {
        // if hi is between hStart and hEnd, then ki and li will be between
        // kStart, kEnd and lStart, lEnd and momi will be between kfmin and
        // kfmax
        double ki = fk * (hi - hStart) + kStart;
        double li = fl * (hi - hStart) + lStart;
        if ((ki >= m_kX[0]) && (ki <= m_kX[kNBins - 1]) && (li >= m_lX[0]) && (li <= m_lX[lNBins - 1])) {
          double momi = fmom * (hi - hStart) + kfmin;
          intersections.push_back({{hi, ki, li, momi}});
        }
      }
    }
  }
  // calculate intersections with planes perpendicular to k
  if (fabs(kStart - kEnd) > eps) {
    double fmom = (kfmax - kfmin) / (kEnd - kStart);
    double fh = (hEnd - hStart) / (kEnd - kStart);
    double fl = (lEnd - lStart) / (kEnd - kStart);
    for (size_t i = 0; i < kNBins; i++) {
      double ki = m_kX[i];
      if (((kStart - ki) * (kEnd - ki) < 0)) {
        // if ki is between kStart and kEnd, then hi and li will be between
        // hStart, hEnd and lStart, lEnd and momi will be between kfmin and
        // kfmax
        double hi = fh * (ki - kStart) + hStart;
        double li = fl * (ki - kStart) + lStart;
        if ((hi >= m_hX[0]) && (hi <= m_hX[hNBins - 1]) && (li >= m_lX[0]) && (li <= m_lX[lNBins - 1])) {
          double momi = fmom * (ki - kStart) + kfmin;
          intersections.push_back({{hi, ki, li, momi}});
        }
      }
    }
  }

  // calculate intersections with planes perpendicular to l
  if (fabs(lStart - lEnd) > eps) {
    double fmom = (kfmax - kfmin) / (lEnd - lStart);
    double fh = (hEnd - hStart) / (lEnd - lStart);
    double fk = (kEnd - kStart) / (lEnd - lStart);

    for (size_t i = 0; i < lNBins; i++) {
      double li = m_lX[i];
      if (((lStart - li) * (lEnd - li) < 0)) {
        double hi = fh * (li - lStart) + hStart;
        double ki = fk * (li - lStart) + kStart;
        if ((hi >= m_hX[0]) && (hi <= m_hX[hNBins - 1]) && (ki >= m_kX[0]) && (ki <= m_kX[kNBins - 1])) {
          double momi = fmom * (li - lStart) + kfmin;
          intersections.push_back({{hi, ki, li, momi}});
        }
      }
    }
  }
  // intersections with dE
  if (!m_dEIntegrated) {
    for (size_t i = 0; i < eNBins; i++) {
      double kfi = m_eX[i];
      if ((kfi - kfmin) * (kfi - kfmax) <= 0) {
        double h = qin.X() * kimin - qout.X() * kfi;
        double k = qin.Y() * kimin - qout.Y() * kfi;
        double l = qin.Z() * kimin - qout.Z() * kfi;
        if ((h >= m_hX[0]) && (h <= m_hX[hNBins - 1]) && (k >= m_kX[0]) && (k <= m_kX[kNBins - 1]) && (l >= m_lX[0]) &&
            (l <= m_lX[lNBins - 1])) {
          intersections.push_back({{h, k, l, kfi}});
        }
      }
    }
  }

  // endpoints
  if ((hStart >= m_hX[0]) && (hStart <= m_hX[hNBins - 1]) && (kStart >= m_kX[0]) && (kStart <= m_kX[kNBins - 1]) &&
      (lStart >= m_lX[0]) && (lStart <= m_lX[lNBins - 1])) {
    intersections.push_back({{hStart, kStart, lStart, kfmin}});
  }
  if ((hEnd >= m_hX[0]) && (hEnd <= m_hX[hNBins - 1]) && (kEnd >= m_kX[0]) && (kEnd <= m_kX[kNBins - 1]) &&
      (lEnd >= m_lX[0]) && (lEnd <= m_lX[lNBins - 1])) {
    intersections.push_back({{hEnd, kEnd, lEnd, kfmax}});
  }

  // sort intersections by final momentum
  std::stable_sort(intersections.begin(), intersections.end(), compareMomentum);
}

/**
 * Linearly interpolate between the points in integrFlux at xValues and save the
 * results in yValues.
 * @param xValues :: X-values at which to interpolate
 * @param integrFlux :: A workspace with the spectra to interpolate
 * @param sp :: A workspace index for a spectrum in integrFlux to interpolate.
 * @param yValues :: A vector to save the results.
 */
void MDNorm::calcIntegralsForIntersections(const std::vector<double> &xValues, const API::MatrixWorkspace &integrFlux,
                                           size_t sp, std::vector<double> &yValues) {
  assert(xValues.size() == yValues.size());

  // the x-data from the workspace
  const auto &xData = integrFlux.x(sp);
  const double xStart = xData.front();
  const double xEnd = xData.back();

  // the values in integrFlux are expected to be integrals of a non-negative
  // function
  // ie they must make a non-decreasing function
  const auto &yData = integrFlux.y(sp);
  size_t spSize = yData.size();

  const double yMin = 0.0;
  const double yMax = yData.back();

  size_t nData = xValues.size();
  // all integrals below xStart must be 0
  if (xValues[nData - 1] < xStart) {
    std::fill(yValues.begin(), yValues.end(), yMin);
    return;
  }

  // all integrals above xEnd must be equal tp yMax
  if (xValues[0] > xEnd) {
    std::fill(yValues.begin(), yValues.end(), yMax);
    return;
  }

  size_t i = 0;
  // integrals below xStart must be 0
  while (i < nData - 1 && xValues[i] < xStart) {
    yValues[i] = yMin;
    i++;
  }
  size_t j = 0;
  for (; i < nData; i++) {
    // integrals above xEnd must be equal tp yMax
    if (j >= spSize - 1) {
      yValues[i] = yMax;
    } else {
      double xi = xValues[i];
      while (j < spSize - 1 && xi > xData[j])
        j++;
      // if x falls onto an interpolation point return the corresponding y
      if (xi == xData[j]) {
        yValues[i] = yData[j];
      } else if (j == spSize - 1) {
        // if we get above xEnd it's yMax
        yValues[i] = yMax;
      } else if (j > 0) {
        // interpolate between the consecutive points
        double x0 = xData[j - 1];
        double x1 = xData[j];
        double y0 = yData[j - 1];
        double y1 = yData[j];
        yValues[i] = y0 + (y1 - y0) * (xi - x0) / (x1 - x0);
      } else // j == 0
      {
        yValues[i] = yMin;
      }
    }
  }
}

} // namespace Mantid::MDAlgorithms
