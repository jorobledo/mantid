// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SetUncertainties.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/VisibleWhenProperty.h"

#include <algorithm>
#include <vector>

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SetUncertainties)

using namespace Kernel;
using namespace API;

namespace {
/// Used to compare signal to zero
const double TOLERANCE = 1.e-10;

const std::string ZERO("zero");
const std::string SQRT("sqrt");
const std::string ONE_IF_ZERO("oneIfZero");
const std::string SQRT_OR_ONE("sqrtOrOne");
const std::string CUSTOM("custom");

struct SetError {
  explicit SetError(const double setTo, const double ifEqualTo, const double tolerance)
      : valueToSet(setTo), valueToCompare(ifEqualTo), tolerance(tolerance) {}

  double operator()(const double error) {
    double deviation = error - valueToCompare;
    if (deviation < tolerance && deviation >= 0) {
      return valueToSet;
    } else {
      return error;
    }
  }

  double valueToSet;
  double valueToCompare;
  double tolerance;
};

struct SqrtError {
  explicit SqrtError(const double constant) : zeroSqrtValue(constant) {}

  double operator()(const double intensity) {
    const double localIntensity = fabs(intensity);
    if (localIntensity > TOLERANCE) {
      return sqrt(localIntensity);
    } else {
      return zeroSqrtValue;
    }
  }

  double zeroSqrtValue;
};
} // namespace

/// Algorithm's name
const std::string SetUncertainties::name() const { return "SetUncertainties"; }

/// Algorithm's version
int SetUncertainties::version() const { return (1); }

void SetUncertainties::init() {
  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  auto mustBePositiveInt = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  mustBePositiveInt->setLower(0);
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>("InputWorkspace", "", Direction::Input));
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>("OutputWorkspace", "", Direction::Output));
  std::vector<std::string> errorTypes = {ZERO, SQRT, SQRT_OR_ONE, ONE_IF_ZERO, CUSTOM};
  declareProperty("SetError", ZERO, std::make_shared<StringListValidator>(errorTypes),
                  "How to reset the uncertainties");
  declareProperty("SetErrorTo", 1.000, mustBePositive, "The error value to set when using custom mode");
  setPropertySettings("SetErrorTo", std::make_unique<VisibleWhenProperty>("SetError", IS_EQUAL_TO, "custom"));

  declareProperty("IfEqualTo", 0.000, mustBePositive,
                  "Which error values in the input workspace should be "
                  "replaced when using custom mode");
  setPropertySettings("IfEqualTo", std::make_unique<VisibleWhenProperty>("SetError", IS_EQUAL_TO, "custom"));

  declareProperty("Precision", 3, mustBePositiveInt,
                  "How many decimal places of ``IfEqualTo`` are taken into "
                  "account for matching when using custom mode");
  setPropertySettings("Precision", std::make_unique<VisibleWhenProperty>("SetError", IS_EQUAL_TO, "custom"));
}

void SetUncertainties::exec() {
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  auto inputEventWorkspace = std::dynamic_pointer_cast<const DataObjects::EventWorkspace>(inputWorkspace);
  MatrixWorkspace_sptr outputWorkspace = getProperty("OutputWorkspace");
  std::string errorType = getProperty("SetError");
  bool zeroError = (errorType == ZERO);
  bool takeSqrt = ((errorType == SQRT) || (errorType == SQRT_OR_ONE));
  bool resetOne = ((errorType == ONE_IF_ZERO) || (errorType == SQRT_OR_ONE));
  bool customError = (errorType == CUSTOM);

  double valueToSet = resetOne ? 1.0 : getProperty("SetErrorTo");
  double valueToCompare = resetOne ? 0.0 : getProperty("IfEqualTo");
  int precision = getProperty("Precision");
  double tolerance = resetOne ? 1E-10 : std::pow(10.0, -1. * precision);

  // Create the output workspace. This will copy many aspects from the input
  // one.

  const int64_t numHists = static_cast<int64_t>(inputWorkspace->getNumberHistograms());
  if (inputEventWorkspace) {
    if (inputWorkspace == outputWorkspace && errorType == SQRT &&
        inputEventWorkspace->getEventType() == Mantid::API::EventType::TOF) {
      // Uncertainty is already square root of the y value
      return;
    }
    // Copy the histogram representation over to a Workspace2D
    outputWorkspace = WorkspaceFactory::Instance().create(inputWorkspace);
    PARALLEL_FOR_IF(Kernel::threadSafe(*inputWorkspace, *outputWorkspace))
    for (int64_t i = 0; i < numHists; ++i) {
      PARALLEL_START_INTERUPT_REGION
      // copy  X/Y/E
      outputWorkspace->setSharedX(i, inputWorkspace->sharedX(i));
      outputWorkspace->setSharedY(i, inputWorkspace->sharedY(i));
      outputWorkspace->setSharedE(i, inputWorkspace->sharedE(i));
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
  } else if (inputWorkspace != outputWorkspace) {
    outputWorkspace = inputWorkspace->clone();
  }

  const auto &spectrumInfo = inputWorkspace->spectrumInfo();
  Progress prog(this, 0.0, 1.0, numHists);
  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWorkspace, *outputWorkspace))
  for (int64_t i = 0; i < numHists; ++i) {
    PARALLEL_START_INTERUPT_REGION
    // copy the E or set to zero depending on the mode
    if (errorType == ONE_IF_ZERO || customError) {
    } else {
      outputWorkspace->mutableE(i) = 0.0;
    }
    // ZERO mode doesn't calculate anything further
    if ((!zeroError) && (!(spectrumInfo.hasDetectors(i) && spectrumInfo.isMasked(i)))) {
      auto &E = outputWorkspace->mutableE(i);
      if (takeSqrt) {
        const auto &Y = outputWorkspace->y(i);
        std::transform(Y.begin(), Y.end(), E.begin(), SqrtError(resetOne ? 1. : 0.));
      } else {
        std::transform(E.begin(), E.end(), E.begin(), SetError(valueToSet, valueToCompare, tolerance));
      }
    }
    prog.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  setProperty("OutputWorkspace", outputWorkspace);
}

} // namespace Mantid::Algorithms
