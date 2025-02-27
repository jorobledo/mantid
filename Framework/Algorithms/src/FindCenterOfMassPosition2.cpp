// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/FindCenterOfMassPosition2.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/PhysicalConstants.h"
namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FindCenterOfMassPosition2)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

namespace {
bool equals(const double a, const double b) { // anonymous namespace
  return fabs(a - b) < std::numeric_limits<double>::min();
} // anonymous namespace
} // namespace

void FindCenterOfMassPosition2::init() {
  const auto wsValidator = std::make_shared<CompositeValidator>();
  const auto positiveDouble = std::make_shared<BoundedValidator<double>>();

  wsValidator->add<HistogramValidator>();
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, wsValidator));
  declareProperty("Output", "",
                  "If not empty, a table workspace of that "
                  "name will contain the center of mass position.");

  declareProperty("CenterX", 0.0, "Estimate for the beam center in X [m]. Default: 0");
  declareProperty("CenterY", 0.0, "Estimate for the beam center in Y [m]. Default: 0");
  declareProperty("Tolerance", 0.00125,
                  "Tolerance on the center of mass "
                  "position between each iteration [m]. "
                  "Default: 0.00125");

  declareProperty("DirectBeam", true,
                  "If true, a direct beam calculation will be performed. Otherwise, the "
                  "center of mass "
                  "of the scattering data will be computed by excluding the beam area.");

  positiveDouble->setLower(0);
  declareProperty("BeamRadius", 0.0155, positiveDouble,
                  "Radius of the beam area, in meters, used the exclude the "
                  "beam when calculating "
                  "the center of mass of the scattering pattern.");
}

// find the min/max for x/y coords in set of valid spectra, update position of bounding box
double initBoundingBox(WorkspaceBoundingBox &boundingBox, const int numSpec, const double beamRadius,
                       const bool directBeam) {
  double totalCount = 0;
  for (int i = 0; i < numSpec; i++) {
    if (!boundingBox.isValidWs(i))
      continue;

    boundingBox.updateMinMax(i);

    if (!boundingBox.isOutOfBoundsOfNonDirectBeam(beamRadius, i, directBeam))
      continue;
    else
      totalCount += boundingBox.updatePositionAndReturnCount(i);
  }
  return totalCount;
}

// In subsequent iterations check if spectra fits in the normalized bounding box(generated by previous iterations)
// If so, update position
double updateBoundingBox(WorkspaceBoundingBox &boundingBox, WorkspaceBoundingBox &previousBoundingBox,
                         const int numSpec, const double beamRadius, const bool directBeam) {
  double totalCount = 0;
  const auto &spectrumInfo = boundingBox.getWorkspace()->spectrumInfo();
  for (int i = 0; i < numSpec; i++) {
    if (!boundingBox.isValidWs(i))
      continue;

    const V3D position = spectrumInfo.position(i);

    if (previousBoundingBox.containsPoint(position.X(), position.Y())) {
      if (!boundingBox.isOutOfBoundsOfNonDirectBeam(beamRadius, i, directBeam))
        continue;
      else
        totalCount += boundingBox.updatePositionAndReturnCount(i);
    }
  }
  return totalCount;
}

/** Iterates through spectrum in the input workspace finding the center of mass until
 *  we converge to within the tolerance specified in meters
 *
 *  @param inputWS  :: workspace to find the center of mass of
 *  @param centerX  :: save the real center of mass x coord here
 *  @param centerY  :: save the real center of mass y coord here
 *  @param numSpec  :: number of spectrum in the workspace to iterate through
 *  @param progress :: object for reporting progress of the operation
 */
void FindCenterOfMassPosition2::findCenterOfMass(const API::MatrixWorkspace_sptr &inputWS, double &centerX,
                                                 double &centerY, const int numSpec, Progress &progress) {
  const double tolerance = getProperty("Tolerance");
  const bool directBeam = getProperty("DirectBeam");
  const double beamRadius = getProperty("BeamRadius");

  // Define box around center of mass so that only pixels in an area
  // _centered_ on the latest center position are considered. At each
  // iteration we will recompute the bounding box, and we will make
  // it as large as possible. The largest box is defined in:
  WorkspaceBoundingBox boundingBox(inputWS);
  boundingBox.setCenter(centerX, centerY);

  // Starting values for the bounding box and the center
  WorkspaceBoundingBox previousBoundingBox;
  previousBoundingBox.setBounds(0., 0., 0., 0.);

  // Initialize book-keeping
  double distance = -1;
  double distanceCheck = 0;
  double totalCount = initBoundingBox(boundingBox, numSpec, beamRadius, directBeam);

  int totalLocalMinima = 0;
  int totalIterations = 0;

  // Find center of mass and iterate until we converge
  // to within the tolerance specified in meters
  while (distance > tolerance || distance < 0) {
    // Normalize output to find center of mass position
    boundingBox.normalizePosition(totalCount, totalCount);
    // Compute the distance to the previous iteration
    distance = boundingBox.calculateDistance();
    // Recenter around new mass position
    double radiusX = boundingBox.calculateRadiusX();
    double radiusY = boundingBox.calculateRadiusY();

    if (!directBeam && (radiusX <= beamRadius || radiusY <= beamRadius)) {
      g_log.error() << "Center of mass falls within the beam center area: "
                       "stopping here\n";
      break;
    }

    boundingBox.setCenter(boundingBox.getX(), boundingBox.getY());
    const auto oldCenterX = boundingBox.getCenterX();
    const auto oldCenterY = boundingBox.getCenterY();
    previousBoundingBox.setBounds(oldCenterX - radiusX, oldCenterX + radiusX, oldCenterY - radiusY,
                                  oldCenterY + radiusY);

    // Check to see if we have the same result
    // as the previous iteration
    if (equals(distance, distanceCheck)) {
      totalLocalMinima++;
    } else {
      totalLocalMinima = 0;
    }

    // Quit if we found the exact same distance five times in a row.
    if (totalLocalMinima > 5) {
      g_log.warning() << "Found the same or equivalent center of mass locations "
                         "more than 5 times in a row: stopping here\n";
      break;
    }

    // Quit if we haven't converged after the maximum number of iterations.
    if (++totalIterations > m_maxIteration) {
      g_log.warning() << "More than " << m_maxIteration << " iteration to find beam center: stopping here\n";
      break;
    }

    distanceCheck = distance;

    // Count histogram for normalization
    boundingBox.setPosition(0, 0);
    totalCount = updateBoundingBox(boundingBox, previousBoundingBox, numSpec, beamRadius, directBeam);

    progress.report("Find Beam Center");
  }
  centerX = boundingBox.getCenterX();
  centerY = boundingBox.getCenterY();
}

/** Package the algorithm outputs one of two ways depending on whether or
 *  not it was given an input EventWorkspace to start with
 *
 *  @param centerX  :: center of mass x coord to package
 *  @param centerY  :: center of mass y coord to package
 */
void FindCenterOfMassPosition2::storeOutputWorkspace(double centerX, double centerY) {
  std::string output = getProperty("Output");

  // If an output workspace name was given, create a TableWorkspace with the
  // results,
  // otherwise use an ArrayProperty
  if (!output.empty()) {
    // Store the result in a table workspace
    declareProperty(
        std::make_unique<WorkspaceProperty<API::ITableWorkspace>>("OutputWorkspace", "", Direction::Output));

    // Set the name of the new workspace
    setPropertyValue("OutputWorkspace", output);

    Mantid::API::ITableWorkspace_sptr m_result = std::make_shared<TableWorkspace>();
    m_result->addColumn("str", "Name");
    m_result->addColumn("double", "Value");

    Mantid::API::TableRow row = m_result->appendRow();
    row << "X (m)" << centerX;
    row = m_result->appendRow();
    row << "Y (m)" << centerY;

    setProperty("OutputWorkspace", m_result);
  } else {
    // Store the results using an ArrayProperty
    if (!existsProperty("CenterOfMass"))
      declareProperty(std::make_unique<ArrayProperty<double>>("CenterOfMass", std::make_shared<NullValidator>(),
                                                              Direction::Output));
    std::vector<double> center_of_mass;
    center_of_mass.emplace_back(centerX);
    center_of_mass.emplace_back(centerY);
    setProperty("CenterOfMass", center_of_mass);
  }

  g_log.information() << "Center of Mass found at x=" << centerX << " y=" << centerY << '\n';
}

void FindCenterOfMassPosition2::exec() {
  const MatrixWorkspace_sptr inputWSWvl = getProperty("InputWorkspace");
  double centerX = getProperty("CenterX");
  double centerY = getProperty("CenterY");

  MatrixWorkspace_sptr inputWS;

  // Sum up all the wavelength bins
  IAlgorithm_sptr childAlg = createChildAlgorithm("Integration", 0., 0.5); // first half is integrating
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWSWvl);
  childAlg->executeAsChildAlg();
  inputWS = childAlg->getProperty("OutputWorkspace");

  // Get the number of monitors. We assume that all monitors are stored in the
  // first spectra
  const auto numSpec = static_cast<int>(inputWSWvl->getNumberHistograms());

  // Set up the progress reporting object
  Progress progress(this, 0.5, 1.0, m_maxIteration);

  findCenterOfMass(inputWS, centerX, centerY, numSpec, progress);
  storeOutputWorkspace(centerX, centerY);
}

} // namespace Mantid::Algorithms
