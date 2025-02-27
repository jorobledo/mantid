// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/FilterByLogValue.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ITimeSeriesProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"

namespace Mantid::Algorithms {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(FilterByLogValue)

using namespace Kernel;
using namespace DataObjects;
using namespace API;
using DataObjects::EventList;
using DataObjects::EventWorkspace;
using DataObjects::EventWorkspace_const_sptr;
using DataObjects::EventWorkspace_sptr;
using Types::Core::DateAndTime;

std::string CENTRE("Centre");
std::string LEFT("Left");

void FilterByLogValue::init() {
  declareProperty(std::make_unique<WorkspaceProperty<EventWorkspace>>("InputWorkspace", "", Direction::Input),
                  "An input event workspace");

  declareProperty(std::make_unique<WorkspaceProperty<EventWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The name to use for the output workspace");

  declareProperty("LogName", "", std::make_shared<MandatoryValidator<std::string>>(),
                  "Name of the sample log to use to filter.\n"
                  "For example, the pulse charge is recorded in 'ProtonCharge'.");

  declareProperty("MinimumValue", Mantid::EMPTY_DBL(), "Minimum log value for which to keep events.");

  declareProperty("MaximumValue", Mantid::EMPTY_DBL(), "Maximum log value for which to keep events.");

  auto min = std::make_shared<BoundedValidator<double>>();
  min->setLower(0.0);
  declareProperty("TimeTolerance", 0.0, min,
                  "Tolerance, in seconds, for the event times to keep. How TimeTolerance is applied is highly "
                  "correlated to LogBoundary and PulseFilter.  Check the help or algorithm documents for details.");

  std::vector<std::string> types(2);
  types[0] = CENTRE;
  types[1] = LEFT;
  declareProperty("LogBoundary", types[0], std::make_shared<StringListValidator>(types),
                  "How to treat log values as being measured in the centre of "
                  "the time window for which log criteria are satisfied, or left (beginning) of time window boundary. "
                  "This value must be set to Left if the sample log is recorded upon changing,"
                  "which applies to most of the sample environment devices in SNS.");

  declareProperty("PulseFilter", false,
                  "Optional. Filter out a notch of time for each entry in the "
                  "sample log named.\n"
                  "A notch of width 2*TimeTolerance is centered at each log "
                  "time. The value of the log is NOT used."
                  "This is used, for example, to filter out veto pulses.");
}

std::map<std::string, std::string> FilterByLogValue::validateInputs() {
  std::map<std::string, std::string> errors;

  // check for null pointers - this is to protect against workspace groups
  EventWorkspace_const_sptr inputWS = this->getProperty("InputWorkspace");
  if (!inputWS) {
    return errors;
  }

  // Check that the log exists for the given input workspace
  std::string logname = getPropertyValue("LogName");
  try {
    auto *log = dynamic_cast<ITimeSeriesProperty *>(inputWS->run().getLogData(logname));
    if (log == nullptr) {
      errors["LogName"] = "'" + logname + "' is not a time-series log.";
      return errors;
    }
  } catch (Exception::NotFoundError &) {
    errors["LogName"] = "The log '" + logname + "' does not exist in the workspace '" + inputWS->getName() + "'.";
    return errors;
  }

  const double min = getProperty("MinimumValue");
  const double max = getProperty("MaximumValue");
  if (!isEmpty(min) && !isEmpty(max) && (max < min)) {
    errors["MinimumValue"] = "MinimumValue must not be larger than MaximumValue";
    errors["MaximumValue"] = "MinimumValue must not be larger than MaximumValue";
  }

  return errors;
}

/** Executes the algorithm
 */
void FilterByLogValue::exec() {

  // convert the input workspace into the event workspace we already know it is
  EventWorkspace_sptr inputWS = this->getProperty("InputWorkspace");

  // Get the properties.
  double min = getProperty("MinimumValue");
  double max = getProperty("MaximumValue");
  const double tolerance = getProperty("TimeTolerance");
  const std::string logname = getPropertyValue("LogName");
  const bool PulseFilter = getProperty("PulseFilter");

  // Find the start and stop times of the run, but handle it if they are not
  // found.
  DateAndTime run_start(0), run_stop("2100-01-01T00:00:00");
  bool handle_edge_values = false;
  try {
    run_start = inputWS->getFirstPulseTime() - tolerance;
    run_stop = inputWS->getLastPulseTime() + tolerance;
    handle_edge_values = true;
  } catch (Exception::NotFoundError &) {
  }

  // Now make the splitter vector
  TimeSplitterType splitter;
  // This'll throw an exception if the log doesn't exist. That is good.
  auto *log = dynamic_cast<ITimeSeriesProperty *>(inputWS->run().getLogData(logname));
  if (log) {
    if (PulseFilter) {
      // ----- Filter at pulse times only -----
      DateAndTime lastTime = run_start;
      std::vector<DateAndTime> times = log->timesAsVector();
      std::vector<DateAndTime>::iterator it;
      for (it = times.begin(); it != times.end(); ++it) {
        SplittingInterval interval(lastTime, *it - tolerance, 0);
        // Leave a gap +- tolerance
        lastTime = (*it + tolerance);
        splitter.emplace_back(interval);
      }
      // And the last one
      splitter.emplace_back(lastTime, run_stop, 0);

    } else {
      // ----- Filter by value ------

      // This function creates the splitter vector we will use to filter out
      // stuff.
      const std::string logBoundary(this->getPropertyValue("LogBoundary"));
      log->makeFilterByValue(splitter, min, max, tolerance, (logBoundary == CENTRE));

      if (log->realSize() >= 1 && handle_edge_values) {
        log->expandFilterToRange(splitter, min, max, TimeInterval(run_start, run_stop));
      }
    } // (filter by value)
  }

  g_log.information() << splitter.size() << " entries in the filter.\n";
  size_t numberOfSpectra = inputWS->getNumberHistograms();

  // Initialise the progress reporting object
  Progress prog(this, 0.0, 1.0, numberOfSpectra);

  EventWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (inputWS == outputWS) {
    // Filtering in place!
    // -------------------------------------------------------------
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int64_t i = 0; i < int64_t(numberOfSpectra); ++i) {
      PARALLEL_START_INTERUPT_REGION

      // this is the input event list
      EventList &input_el = inputWS->getSpectrum(i);

      // Perform the filtering in place.
      input_el.filterInPlace(splitter);

      prog.report();
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    // To split/filter the runs, first you make a vector with just the one
    // output run
    auto newRun = Kernel::make_cow<Run>(inputWS->run());
    std::vector<LogManager *> splitRuns = {&newRun.access()};
    inputWS->run().splitByTime(splitter, splitRuns);
    // Set the output back in the input
    inputWS->setSharedRun(newRun);
    inputWS->mutableRun().integrateProtonCharge();

    // Cast the outputWS to the matrixOutputWS and save it
    this->setProperty("OutputWorkspace", inputWS);
  } else {
    // Make a brand new EventWorkspace for the output
    outputWS = create<EventWorkspace>(*inputWS);

    // Loop over the histograms (detector spectra)
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int64_t i = 0; i < int64_t(numberOfSpectra); ++i) {
      PARALLEL_START_INTERUPT_REGION

      // Get the output event list (should be empty)
      std::vector<EventList *> outputs{&outputWS->getSpectrum(i)};

      // and this is the input event list
      const EventList &input_el = inputWS->getSpectrum(i);

      // Perform the filtering (using the splitting function and just one
      // output)
      input_el.splitByTime(splitter, outputs);

      prog.report();
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    // To split/filter the runs, first you make a vector with just the one
    // output run
    std::vector<LogManager *> output_runs;
    output_runs.emplace_back(&outputWS->mutableRun());
    inputWS->run().splitByTime(splitter, output_runs);

    // Cast the outputWS to the matrixOutputWS and save it
    this->setProperty("OutputWorkspace", outputWS);
  }
}

} // namespace Mantid::Algorithms
