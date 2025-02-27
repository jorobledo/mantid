// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FitPeaks.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAlgorithms/FindPeakBackground.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidHistogramData/EstimatePolynomial.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramBuilder.h"
#include "MantidHistogramData/HistogramIterator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/IValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/StartsWithValidator.h"

#include "boost/algorithm/string.hpp"
#include "boost/algorithm/string/trim.hpp"
#include <limits>
#include <utility>

using namespace Mantid;
using namespace Algorithms::PeakParameterHelper;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using Mantid::HistogramData::Histogram;
using namespace std;

namespace Mantid::Algorithms {

namespace {
namespace PropertyNames {
const std::string INPUT_WKSP("InputWorkspace");
const std::string OUTPUT_WKSP("OutputWorkspace");
const std::string START_WKSP_INDEX("StartWorkspaceIndex");
const std::string STOP_WKSP_INDEX("StopWorkspaceIndex");
const std::string PEAK_CENTERS("PeakCenters");
const std::string PEAK_CENTERS_WKSP("PeakCentersWorkspace");
const std::string PEAK_FUNC("PeakFunction");
const std::string BACK_FUNC("BackgroundType");
const std::string FIT_WINDOW_LIST("FitWindowBoundaryList");
const std::string FIT_WINDOW_WKSP("FitPeakWindowWorkspace");
const std::string PEAK_WIDTH_PERCENT("PeakWidthPercent");
const std::string PEAK_PARAM_NAMES("PeakParameterNames");
const std::string PEAK_PARAM_VALUES("PeakParameterValues");
const std::string PEAK_PARAM_TABLE("PeakParameterValueTable");
const std::string FIT_FROM_RIGHT("FitFromRight");
const std::string MINIMIZER("Minimizer");
const std::string COST_FUNC("CostFunction");
const std::string MAX_FIT_ITER("MaxFitIterations");
const std::string BACKGROUND_Z_SCORE("FindBackgroundSigma");
const std::string HIGH_BACKGROUND("HighBackground");
const std::string POSITION_TOL("PositionTolerance");
const std::string PEAK_MIN_HEIGHT("MinimumPeakHeight");
const std::string CONSTRAIN_PEAK_POS("ConstrainPeakPositions");
const std::string OUTPUT_WKSP_MODEL("FittedPeaksWorkspace");
const std::string OUTPUT_WKSP_PARAMS("OutputPeakParametersWorkspace");
const std::string OUTPUT_WKSP_PARAM_ERRS("OutputParameterFitErrorsWorkspace");
const std::string RAW_PARAMS("RawPeakParameters");

} // namespace PropertyNames
} // namespace

namespace FitPeaksAlgorithm {

//----------------------------------------------------------------------------------------------
/// Holds all of the fitting information for a single spectrum
PeakFitResult::PeakFitResult(size_t num_peaks, size_t num_params) : m_function_parameters_number(num_params) {
  // check input
  if (num_peaks == 0 || num_params == 0)
    throw std::runtime_error("No peak or no parameter error.");

  //
  m_fitted_peak_positions.resize(num_peaks, std::numeric_limits<double>::quiet_NaN());
  m_costs.resize(num_peaks, DBL_MAX);
  m_function_parameters_vector.resize(num_peaks);
  m_function_errors_vector.resize(num_peaks);
  for (size_t ipeak = 0; ipeak < num_peaks; ++ipeak) {
    m_function_parameters_vector[ipeak].resize(num_params, std::numeric_limits<double>::quiet_NaN());
    m_function_errors_vector[ipeak].resize(num_params, std::numeric_limits<double>::quiet_NaN());
  }

  return;
}

//----------------------------------------------------------------------------------------------
size_t PeakFitResult::getNumberParameters() const { return m_function_parameters_number; }

size_t PeakFitResult::getNumberPeaks() const { return m_function_parameters_vector.size(); }

//----------------------------------------------------------------------------------------------
/** get the fitting error of a particular parameter
 * @param ipeak :: index of the peak in given peak position vector
 * @param iparam :: index of the parameter in its corresponding peak profile
 * function
 * @return :: fitting error/uncertain of the specified parameter
 */
double PeakFitResult::getParameterError(size_t ipeak, size_t iparam) const {
  return m_function_errors_vector[ipeak][iparam];
}

//----------------------------------------------------------------------------------------------
/** get the fitted value of a particular parameter
 * @param ipeak :: index of the peak in given peak position vector
 * @param iparam :: index of the parameter in its corresponding peak profile
 * function
 * @return :: fitted value of the specified parameter
 */
double PeakFitResult::getParameterValue(size_t ipeak, size_t iparam) const {
  return m_function_parameters_vector[ipeak][iparam];
}

//----------------------------------------------------------------------------------------------
double PeakFitResult::getPeakPosition(size_t ipeak) const { return m_fitted_peak_positions[ipeak]; }

//----------------------------------------------------------------------------------------------
double PeakFitResult::getCost(size_t ipeak) const { return m_costs[ipeak]; }

//----------------------------------------------------------------------------------------------
/// set the peak fitting record/parameter for one peak
void PeakFitResult::setRecord(size_t ipeak, const double cost, const double peak_position,
                              const FitFunction &fit_functions) {
  // check input
  if (ipeak >= m_costs.size())
    throw std::runtime_error("Peak index is out of range.");

  // set the values
  m_costs[ipeak] = cost;

  // set peak position
  m_fitted_peak_positions[ipeak] = peak_position;

  // transfer from peak function to vector
  size_t peak_num_params = fit_functions.peakfunction->nParams();
  for (size_t ipar = 0; ipar < peak_num_params; ++ipar) {
    // peak function
    m_function_parameters_vector[ipeak][ipar] = fit_functions.peakfunction->getParameter(ipar);
    m_function_errors_vector[ipeak][ipar] = fit_functions.peakfunction->getError(ipar);
  }
  for (size_t ipar = 0; ipar < fit_functions.bkgdfunction->nParams(); ++ipar) {
    // background function
    m_function_parameters_vector[ipeak][ipar + peak_num_params] = fit_functions.bkgdfunction->getParameter(ipar);
    m_function_errors_vector[ipeak][ipar + peak_num_params] = fit_functions.bkgdfunction->getError(ipar);
  }
}

//----------------------------------------------------------------------------------------------
/** The peak postition should be negative and indicates what went wrong
 * @param ipeak :: index of the peak in user-specified peak position vector
 * @param peak_position :: bad peak position indicating reason of bad fit
 */
void PeakFitResult::setBadRecord(size_t ipeak, const double peak_position) {
  // check input
  if (ipeak >= m_costs.size())
    throw std::runtime_error("Peak index is out of range");
  if (peak_position >= 0.)
    throw std::runtime_error("Can only set negative postion for bad record");

  // set the values
  m_costs[ipeak] = DBL_MAX;

  // set peak position
  m_fitted_peak_positions[ipeak] = peak_position;

  // transfer from peak function to vector
  for (size_t ipar = 0; ipar < m_function_parameters_number; ++ipar) {
    m_function_parameters_vector[ipeak][ipar] = 0.;
    m_function_errors_vector[ipeak][ipar] = std::numeric_limits<double>::quiet_NaN();
  }
}
} // namespace FitPeaksAlgorithm

//----------------------------------------------------------------------------------------------
FitPeaks::FitPeaks()
    : m_fitPeaksFromRight(true), m_fitIterations(50), m_numPeaksToFit(0), m_minPeakHeight(20.), m_bkgdSimga(1.),
      m_peakPosTolCase234(false) {}

//----------------------------------------------------------------------------------------------
/** initialize the properties
 */
void FitPeaks::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropertyNames::INPUT_WKSP, "", Direction::Input),
                  "Name of the input workspace for peak fitting.");
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropertyNames::OUTPUT_WKSP, "", Direction::Output),
      "Name of the output workspace containing peak centers for "
      "fitting offset."
      "The output workspace is point data."
      "Each workspace index corresponds to a spectrum. "
      "Each X value ranges from 0 to N-1, where N is the number of "
      "peaks to fit. "
      "Each Y value is the peak position obtained by peak fitting. "
      "Negative value is used for error signals. "
      "-1 for data is zero;  -2 for maximum value is smaller than "
      "specified minimum value."
      "and -3 for non-converged fitting.");

  // properties about fitting range and criteria
  declareProperty(PropertyNames::START_WKSP_INDEX, EMPTY_INT(), "Starting workspace index for fit");
  declareProperty(PropertyNames::STOP_WKSP_INDEX, EMPTY_INT(),
                  "Last workspace index to fit (which is included). "
                  "If a value larger than the workspace index of last spectrum, "
                  "then the workspace index of last spectrum is used.");

  // properties about peak positions to fit
  declareProperty(std::make_unique<ArrayProperty<double>>(PropertyNames::PEAK_CENTERS),
                  "List of peak centers to fit against.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropertyNames::PEAK_CENTERS_WKSP, "",
                                                                       Direction::Input, PropertyMode::Optional),
                  "MatrixWorkspace containing peak centers");

  const std::string peakcentergrp("Peak Positions");
  setPropertyGroup(PropertyNames::PEAK_CENTERS, peakcentergrp);
  setPropertyGroup(PropertyNames::PEAK_CENTERS_WKSP, peakcentergrp);

  // properties about peak profile
  const std::vector<std::string> peakNames = FunctionFactory::Instance().getFunctionNames<API::IPeakFunction>();
  declareProperty(PropertyNames::PEAK_FUNC, "Gaussian", std::make_shared<StringListValidator>(peakNames),
                  "Use of a BackToBackExponential profile is only reccomended if the "
                  "coeficients to calculate A and B are defined in the instrument "
                  "Parameters.xml file.");
  const vector<string> bkgdtypes{"Flat", "Linear", "Quadratic"};
  declareProperty(PropertyNames::BACK_FUNC, "Linear", std::make_shared<StringListValidator>(bkgdtypes),
                  "Type of Background.");

  const std::string funcgroup("Function Types");
  setPropertyGroup(PropertyNames::PEAK_FUNC, funcgroup);
  setPropertyGroup(PropertyNames::BACK_FUNC, funcgroup);

  // properties about peak range including fitting window and peak width
  // (percentage)
  declareProperty(std::make_unique<ArrayProperty<double>>(PropertyNames::FIT_WINDOW_LIST),
                  "List of left boundaries of the peak fitting window corresponding to "
                  "PeakCenters.");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropertyNames::FIT_WINDOW_WKSP, "",
                                                                       Direction::Input, PropertyMode::Optional),
                  "MatrixWorkspace for of peak windows");

  auto min = std::make_shared<BoundedValidator<double>>();
  min->setLower(1e-3);
  // min->setUpper(1.); TODO make this a limit
  declareProperty(PropertyNames::PEAK_WIDTH_PERCENT, EMPTY_DBL(), min,
                  "The estimated peak width as a "
                  "percentage of the d-spacing "
                  "of the center of the peak. Value must be less than 1.");

  const std::string fitrangeegrp("Peak Range Setup");
  setPropertyGroup(PropertyNames::PEAK_WIDTH_PERCENT, fitrangeegrp);
  setPropertyGroup(PropertyNames::FIT_WINDOW_LIST, fitrangeegrp);
  setPropertyGroup(PropertyNames::FIT_WINDOW_WKSP, fitrangeegrp);

  // properties about peak parameters' names and value
  declareProperty(std::make_unique<ArrayProperty<std::string>>(PropertyNames::PEAK_PARAM_NAMES),
                  "List of peak parameters' names");
  declareProperty(std::make_unique<ArrayProperty<double>>(PropertyNames::PEAK_PARAM_VALUES),
                  "List of peak parameters' value");
  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>(PropertyNames::PEAK_PARAM_TABLE, "",
                                                                      Direction::Input, PropertyMode::Optional),
                  "Name of the an optional workspace, whose each column "
                  "corresponds to given peak parameter names"
                  ", and each row corresponds to a subset of spectra.");

  const std::string startvaluegrp("Starting Parameters Setup");
  setPropertyGroup(PropertyNames::PEAK_PARAM_NAMES, startvaluegrp);
  setPropertyGroup(PropertyNames::PEAK_PARAM_VALUES, startvaluegrp);
  setPropertyGroup(PropertyNames::PEAK_PARAM_TABLE, startvaluegrp);

  // optimization setup
  declareProperty(PropertyNames::FIT_FROM_RIGHT, true,
                  "Flag for the order to fit peaks.  If true, peaks are fitted "
                  "from rightmost;"
                  "Otherwise peaks are fitted from leftmost.");

  const std::vector<std::string> minimizerOptions = API::FuncMinimizerFactory::Instance().getKeys();
  declareProperty(PropertyNames::MINIMIZER, "Levenberg-Marquardt",
                  Kernel::IValidator_sptr(new Kernel::StartsWithValidator(minimizerOptions)),
                  "Minimizer to use for fitting.");

  const std::array<string, 2> costFuncOptions = {{"Least squares", "Rwp"}};
  declareProperty(PropertyNames::COST_FUNC, "Least squares",
                  Kernel::IValidator_sptr(new Kernel::ListValidator<std::string>(costFuncOptions)), "Cost functions");

  auto min_max_iter = std::make_shared<BoundedValidator<int>>();
  min_max_iter->setLower(49);
  declareProperty(PropertyNames::MAX_FIT_ITER, 50, min_max_iter, "Maximum number of function fitting iterations.");

  const std::string optimizergrp("Optimization Setup");
  setPropertyGroup(PropertyNames::MINIMIZER, optimizergrp);
  setPropertyGroup(PropertyNames::COST_FUNC, optimizergrp);

  // other helping information
  declareProperty(PropertyNames::BACKGROUND_Z_SCORE, 1.0,
                  "Multiplier of standard deviations of the variance for convergence of "
                  "peak elimination.  Default is 1.0. ");

  declareProperty(PropertyNames::HIGH_BACKGROUND, true,
                  "Flag whether the data has high background comparing to "
                  "peaks' intensities. "
                  "For example, vanadium peaks usually have high background.");

  declareProperty(std::make_unique<ArrayProperty<double>>(PropertyNames::POSITION_TOL),
                  "List of tolerance on fitted peak positions against given peak positions."
                  "If there is only one value given, then ");

  declareProperty(PropertyNames::PEAK_MIN_HEIGHT, 0.,
                  "Minimum peak height such that all the fitted peaks with "
                  "height under this value will be excluded.");

  declareProperty(PropertyNames::CONSTRAIN_PEAK_POS, true,
                  "If true peak position will be constrained by estimated positions "
                  "(highest Y value position) and "
                  "the peak width either estimted by observation or calculate.");

  // additional output for reviewing
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropertyNames::OUTPUT_WKSP_MODEL, "",
                                                                       Direction::Output, PropertyMode::Optional),
                  "Name of the output matrix workspace with fitted peak. "
                  "This output workspace have the same dimesion as the input workspace."
                  "The Y values belonged to peaks to fit are replaced by fitted value. "
                  "Values of estimated background are used if peak fails to be fit.");

  declareProperty(std::make_unique<WorkspaceProperty<API::ITableWorkspace>>(PropertyNames::OUTPUT_WKSP_PARAMS, "",
                                                                            Direction::Output),
                  "Name of table workspace containing all fitted peak parameters.");

  // Optional output table workspace for each individual parameter's fitting
  // error
  declareProperty(std::make_unique<WorkspaceProperty<API::ITableWorkspace>>(PropertyNames::OUTPUT_WKSP_PARAM_ERRS, "",
                                                                            Direction::Output, PropertyMode::Optional),
                  "Name of workspace containing all fitted peak parameters' fitting error."
                  "It must be used along with FittedPeaksWorkspace and RawPeakParameters "
                  "(True)");

  declareProperty(PropertyNames::RAW_PARAMS, true,
                  "false generates table with effective centre/width/height "
                  "parameters. true generates a table with peak function "
                  "parameters");

  const std::string addoutgrp("Analysis");
  setPropertyGroup(PropertyNames::OUTPUT_WKSP_PARAMS, addoutgrp);
  setPropertyGroup(PropertyNames::OUTPUT_WKSP_MODEL, addoutgrp);
  setPropertyGroup(PropertyNames::OUTPUT_WKSP_PARAM_ERRS, addoutgrp);
  setPropertyGroup(PropertyNames::RAW_PARAMS, addoutgrp);
}

//----------------------------------------------------------------------------------------------
/** Validate inputs
 */
std::map<std::string, std::string> FitPeaks::validateInputs() {
  map<std::string, std::string> issues;

  // check that the peak parameters are in parallel properties
  bool haveCommonPeakParameters(false);
  std::vector<string> suppliedParameterNames = getProperty(PropertyNames::PEAK_PARAM_NAMES);
  std::vector<double> peakParamValues = getProperty(PropertyNames::PEAK_PARAM_VALUES);
  if ((!suppliedParameterNames.empty()) || (!peakParamValues.empty())) {
    haveCommonPeakParameters = true;
    if (suppliedParameterNames.size() != peakParamValues.size()) {
      issues[PropertyNames::PEAK_PARAM_NAMES] = "must have same number of values as PeakParameterValues";
      issues[PropertyNames::PEAK_PARAM_VALUES] = "must have same number of values as PeakParameterNames";
    }
  }

  // get the information out of the table
  std::string partablename = getPropertyValue(PropertyNames::PEAK_PARAM_TABLE);
  if (!partablename.empty()) {
    if (haveCommonPeakParameters) {
      const std::string msg = "Parameter value table and initial parameter "
                              "name/value vectors cannot be given "
                              "simultanenously.";
      issues[PropertyNames::PEAK_PARAM_TABLE] = msg;
      issues[PropertyNames::PEAK_PARAM_NAMES] = msg;
      issues[PropertyNames::PEAK_PARAM_VALUES] = msg;
    } else {
      m_profileStartingValueTable = getProperty(PropertyNames::PEAK_PARAM_TABLE);
      suppliedParameterNames = m_profileStartingValueTable->getColumnNames();
    }
  }

  // check that the suggested peak parameter names exist in the peak function
  if (!suppliedParameterNames.empty()) {
    std::string peakfunctiontype = getPropertyValue(PropertyNames::PEAK_FUNC);
    m_peakFunction =
        std::dynamic_pointer_cast<IPeakFunction>(API::FunctionFactory::Instance().createFunction(peakfunctiontype));

    // put the names in a vector
    std::vector<string> functionParameterNames;
    for (size_t i = 0; i < m_peakFunction->nParams(); ++i)
      functionParameterNames.emplace_back(m_peakFunction->parameterName(i));
    // check that the supplied names are in the function
    // it is acceptable to be missing parameters
    bool failed = false;
    for (const auto &name : suppliedParameterNames) {
      if (std::find(functionParameterNames.begin(), functionParameterNames.end(), name) ==
          functionParameterNames.end()) {
        failed = true;
        break;
      }
    }
    if (failed) {
      std::string msg = "Specified invalid parameter for peak function";
      if (haveCommonPeakParameters)
        issues[PropertyNames::PEAK_PARAM_NAMES] = msg;
      else
        issues[PropertyNames::PEAK_PARAM_TABLE] = msg;
    }
  }

  // check inputs for uncertainty (fitting error)
  const std::string error_table_name = getPropertyValue(PropertyNames::OUTPUT_WKSP_PARAM_ERRS);
  if (!error_table_name.empty()) {
    const bool use_raw_params = getProperty(PropertyNames::RAW_PARAMS);
    if (!use_raw_params) {
      issues[PropertyNames::OUTPUT_WKSP_PARAM_ERRS] = "Cannot be used with " + PropertyNames::RAW_PARAMS + "=False";
      issues[PropertyNames::RAW_PARAMS] =
          "Cannot be False with " + PropertyNames::OUTPUT_WKSP_PARAM_ERRS + " specified";
    }
  }

  return issues;
}

//----------------------------------------------------------------------------------------------
void FitPeaks::exec() {
  // process inputs
  processInputs();

  // create output workspace: fitted peak positions
  generateOutputPeakPositionWS();

  // create output workspace: fitted peaks' parameters values
  generateFittedParametersValueWorkspaces();

  // create output workspace: calculated from fitted peak and background
  generateCalculatedPeaksWS();

  // fit peaks
  auto fit_results = fitPeaks();

  // set the output workspaces to properites
  processOutputs(fit_results);
}

//----------------------------------------------------------------------------------------------
void FitPeaks::processInputs() {
  // input workspaces
  m_inputMatrixWS = getProperty(PropertyNames::INPUT_WKSP);

  if (m_inputMatrixWS->getAxis(0)->unit()->unitID() == "dSpacing")
    m_inputIsDSpace = true;
  else
    m_inputIsDSpace = false;

  // spectra to fit
  int start_wi = getProperty(PropertyNames::START_WKSP_INDEX);
  if (isEmpty(start_wi))
    m_startWorkspaceIndex = 0;
  else
    m_startWorkspaceIndex = static_cast<size_t>(start_wi);

  // last spectrum's workspace index, which is included
  int stop_wi = getProperty(PropertyNames::STOP_WKSP_INDEX);
  if (isEmpty(stop_wi))
    m_stopWorkspaceIndex = m_inputMatrixWS->getNumberHistograms() - 1;
  else {
    m_stopWorkspaceIndex = static_cast<size_t>(stop_wi);
    if (m_stopWorkspaceIndex > m_inputMatrixWS->getNumberHistograms() - 1)
      m_stopWorkspaceIndex = m_inputMatrixWS->getNumberHistograms() - 1;
  }

  // optimizer, cost function and fitting scheme
  m_minimizer = getPropertyValue(PropertyNames::MINIMIZER);
  m_costFunction = getPropertyValue(PropertyNames::COST_FUNC);
  m_fitPeaksFromRight = getProperty(PropertyNames::FIT_FROM_RIGHT);
  m_constrainPeaksPosition = getProperty(PropertyNames::CONSTRAIN_PEAK_POS);
  m_fitIterations = getProperty(PropertyNames::MAX_FIT_ITER);

  // Peak centers, tolerance and fitting range
  processInputPeakCenters();
  // check
  if (m_numPeaksToFit == 0)
    throw std::runtime_error("number of peaks to fit is zero.");
  // about how to estimate the peak width
  m_peakWidthPercentage = getProperty(PropertyNames::PEAK_WIDTH_PERCENT);
  if (isEmpty(m_peakWidthPercentage))
    m_peakWidthPercentage = -1;
  if (m_peakWidthPercentage >= 1.) // TODO
    throw std::runtime_error("PeakWidthPercent must be less than 1");
  g_log.debug() << "peak width/value = " << m_peakWidthPercentage << "\n";

  // set up background
  m_highBackground = getProperty(PropertyNames::HIGH_BACKGROUND);
  m_bkgdSimga = getProperty(PropertyNames::BACKGROUND_Z_SCORE);

  // Set up peak and background functions
  processInputFunctions();

  // about peak width and other peak parameter estimating method
  if (m_peakWidthPercentage > 0.)
    m_peakWidthEstimateApproach = EstimatePeakWidth::InstrumentResolution;
  else if (isObservablePeakProfile((m_peakFunction->name())))
    m_peakWidthEstimateApproach = EstimatePeakWidth::Observation;
  else
    m_peakWidthEstimateApproach = EstimatePeakWidth::NoEstimation;
  //  m_peakWidthEstimateApproach = EstimatePeakWidth::NoEstimation;
  g_log.debug() << "Process inputs [3] peak type: " << m_peakFunction->name()
                << ", background type: " << m_bkgdFunction->name() << "\n";

  processInputPeakTolerance();
  processInputFitRanges();

  return;
}

//----------------------------------------------------------------------------------------------
/** process inputs for peak profile and background
 */
void FitPeaks::processInputFunctions() {
  // peak functions
  std::string peakfunctiontype = getPropertyValue(PropertyNames::PEAK_FUNC);
  m_peakFunction =
      std::dynamic_pointer_cast<IPeakFunction>(API::FunctionFactory::Instance().createFunction(peakfunctiontype));

  // background functions
  std::string bkgdfunctiontype = getPropertyValue(PropertyNames::BACK_FUNC);
  std::string bkgdname;
  if (bkgdfunctiontype == "Linear")
    bkgdname = "LinearBackground";
  else if (bkgdfunctiontype == "Flat") {
    g_log.warning("There may be problems with Flat background");
    bkgdname = "FlatBackground";
  } else
    bkgdname = bkgdfunctiontype;
  m_bkgdFunction =
      std::dynamic_pointer_cast<IBackgroundFunction>(API::FunctionFactory::Instance().createFunction(bkgdname));
  if (m_highBackground)
    m_linearBackgroundFunction = std::dynamic_pointer_cast<IBackgroundFunction>(
        API::FunctionFactory::Instance().createFunction("LinearBackground"));
  else
    m_linearBackgroundFunction = nullptr;

  // TODO check that both parameter names and values exist
  // input peak parameters
  std::string partablename = getPropertyValue(PropertyNames::PEAK_PARAM_TABLE);
  m_peakParamNames = getProperty(PropertyNames::PEAK_PARAM_NAMES);

  m_uniformProfileStartingValue = false;
  if (partablename.empty() && (!m_peakParamNames.empty())) {
    // use uniform starting value of peak parameters
    m_initParamValues = getProperty(PropertyNames::PEAK_PARAM_VALUES);
    // convert the parameter name in string to parameter name in integer index
    convertParametersNameToIndex();
    // m_uniformProfileStartingValue = true;
  } else if ((!partablename.empty()) && m_peakParamNames.empty()) {
    // use non-uniform starting value of peak parameters
    m_profileStartingValueTable = getProperty(partablename);
  } else if (peakfunctiontype != "Gaussian") {
    // user specifies nothing
    g_log.warning("Neither parameter value table nor initial "
                  "parameter name/value vectors is specified. Fitting might "
                  "not be reliable for peak profile other than Gaussian");
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** process and check for inputs about peak fitting range (i.e., window)
 * Note: What is the output of the method?
 */
void FitPeaks::processInputFitRanges() {
  // get peak fit window
  std::vector<double> peakwindow = getProperty(PropertyNames::FIT_WINDOW_LIST);
  std::string peakwindowname = getPropertyValue(PropertyNames::FIT_WINDOW_WKSP);
  API::MatrixWorkspace_const_sptr peakwindowws = getProperty(PropertyNames::FIT_WINDOW_WKSP);

  // in most case, calculate window by instrument resolution is False
  m_calculateWindowInstrument = false;

  if ((!peakwindow.empty()) && peakwindowname.empty()) {
    // Peak windows are uniform among spectra: use vector for peak windows
    m_uniformPeakWindows = true;

    // check peak positions
    if (!m_uniformPeakPositions)
      throw std::invalid_argument("Uniform peak range/window requires uniform peak positions.");
    // check size
    if (peakwindow.size() != m_numPeaksToFit * 2)
      throw std::invalid_argument("Peak window vector must be twice as large as number of peaks.");

    // set up window to m_peakWindowVector
    m_peakWindowVector.resize(m_numPeaksToFit);
    for (size_t i = 0; i < m_numPeaksToFit; ++i) {
      std::vector<double> peakranges(2);
      peakranges[0] = peakwindow[i * 2];
      peakranges[1] = peakwindow[i * 2 + 1];
      // check peak window (range) against peak centers
      if ((peakranges[0] < m_peakCenters[i]) && (m_peakCenters[i] < peakranges[1])) {
        // pass check: set
        m_peakWindowVector[i] = peakranges;
      } else {
        // failed
        std::stringstream errss;
        errss << "Peak " << i << ": user specifies an invalid range and peak center against " << peakranges[0] << " < "
              << m_peakCenters[i] << " < " << peakranges[1];
        throw std::invalid_argument(errss.str());
      }
    } // END-FOR
    // END for uniform peak window
  } else if (peakwindow.empty() && peakwindowws != nullptr) {
    // use matrix workspace for non-uniform peak windows
    m_peakWindowWorkspace = getProperty(PropertyNames::FIT_WINDOW_WKSP);
    m_uniformPeakWindows = false;

    // check size
    if (m_peakWindowWorkspace->getNumberHistograms() == m_inputMatrixWS->getNumberHistograms())
      m_partialWindowSpectra = false;
    else if (m_peakWindowWorkspace->getNumberHistograms() == (m_stopWorkspaceIndex - m_startWorkspaceIndex + 1))
      m_partialWindowSpectra = true;
    else
      throw std::invalid_argument("Peak window workspace has unmatched number of spectra");

    // check range for peak windows and peak positions
    size_t window_index_start(0);
    if (m_partialWindowSpectra)
      window_index_start = m_startWorkspaceIndex;
    size_t center_index_start(0);
    if (m_partialSpectra)
      center_index_start = m_startWorkspaceIndex;

    // check each spectrum whether the window is defined with the correct size
    for (size_t wi = 0; wi < m_peakWindowWorkspace->getNumberHistograms(); ++wi) {
      // check size
      if (m_peakWindowWorkspace->y(wi).size() != m_numPeaksToFit * 2) {
        std::stringstream errss;
        errss << "Peak window workspace index " << wi << " has incompatible number of fit windows (x2) "
              << m_peakWindowWorkspace->y(wi).size() << " with the number of peaks " << m_numPeaksToFit << " to fit.";
        throw std::invalid_argument(errss.str());
      }
      const auto &peakWindowX = m_peakWindowWorkspace->x(wi);

      // check window range against peak center
      size_t window_index = window_index_start + wi;
      size_t center_index = window_index - center_index_start;
      const auto &peakCenterX = m_peakCenterWorkspace->x(center_index);

      for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak) {
        double left_w_bound = peakWindowX[ipeak * 2]; // TODO getting on y
        double right_w_bound = peakWindowX[ipeak * 2 + 1];
        double center = peakCenterX[ipeak];
        if (!(left_w_bound < center && center < right_w_bound)) {
          std::stringstream errss;
          errss << "Workspace index " << wi << " has incompatible peak window (" // <<<<<<< HERE!!!!!!!!!
                << left_w_bound << ", " << right_w_bound << ") with " << ipeak << "-th expected peak's center "
                << center;
          throw std::runtime_error(errss.str());
        }
      }
    }
  } else if (peakwindow.empty()) {
    // no peak window is defined, then the peak window will be estimated by
    // delta(D)/D
    if (m_inputIsDSpace && m_peakWidthPercentage > 0)
      m_calculateWindowInstrument = true;
    else
      throw std::invalid_argument("Without definition of peak window, the "
                                  "input workspace must be in unit of dSpacing "
                                  "and Delta(D)/D must be given!");

  } else {
    // non-supported situation
    throw std::invalid_argument("One and only one of peak window array and "
                                "peak window workspace can be specified.");
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Processing peaks centers and fitting tolerance information from input.  the
 * parameters that are
 * set including
 * 1. m_peakCenters/m_peakCenterWorkspace/m_uniformPeakPositions
 * (bool)/m_partialSpectra (bool)
 * 2. m_peakPosTolerances (vector)
 * 3. m_numPeaksToFit
 */
void FitPeaks::processInputPeakCenters() {
  // peak centers
  m_peakCenters = getProperty(PropertyNames::PEAK_CENTERS);
  API::MatrixWorkspace_const_sptr peakcenterws = getProperty(PropertyNames::PEAK_CENTERS_WKSP);
  if (!peakcenterws)
    g_log.notice("Peak centers are not specified by peak center workspace");

  std::string peakpswsname = getPropertyValue(PropertyNames::PEAK_CENTERS_WKSP);
  if ((!m_peakCenters.empty()) && peakcenterws == nullptr) {
    // peak positions are uniform among all spectra
    m_uniformPeakPositions = true;
    // number of peaks to fit!
    m_numPeaksToFit = m_peakCenters.size();
  } else if (m_peakCenters.empty() && peakcenterws != nullptr) {
    // peak positions can be different among spectra
    m_uniformPeakPositions = false;
    m_peakCenterWorkspace = getProperty(PropertyNames::PEAK_CENTERS_WKSP);
    // number of peaks to fit!
    m_numPeaksToFit = m_peakCenterWorkspace->x(0).size();
    g_log.debug() << "Input peak center workspace: " << m_peakCenterWorkspace->x(0).size() << ", "
                  << m_peakCenterWorkspace->y(0).size() << "\n";

    // check matrix worksapce for peak positions
    const size_t peak_center_ws_spectra_number = m_peakCenterWorkspace->getNumberHistograms();
    if (peak_center_ws_spectra_number == m_inputMatrixWS->getNumberHistograms()) {
      // full spectra
      m_partialSpectra = false;
    } else if (peak_center_ws_spectra_number == m_stopWorkspaceIndex - m_startWorkspaceIndex + 1) {
      // partial spectra
      m_partialSpectra = true;
    } else {
      // a case indicating programming error
      g_log.error() << "Peak center workspace has " << peak_center_ws_spectra_number << " spectra;"
                    << "Input workspace has " << m_inputMatrixWS->getNumberHistograms() << " spectra;"
                    << "User specifies to fit peaks from " << m_startWorkspaceIndex << " to " << m_stopWorkspaceIndex
                    << ".  They are mismatched to each other.\n";
      throw std::invalid_argument("Input peak center workspace has mismatched "
                                  "number of spectra to selected spectra to "
                                  "fit.");
    }

  } else {
    std::stringstream errss;
    errss << "One and only one in 'PeakCenters' (vector) and "
             "'PeakCentersWorkspace' shall be given. "
          << "'PeakCenters' has size " << m_peakCenters.size() << ", and name of peak center workspace "
          << "is " << peakpswsname;
    throw std::invalid_argument(errss.str());
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Processing peak fitting tolerance information from input.  The parameters
 * that are
 * set including
 * 2. m_peakPosTolerances (vector)
 */
void FitPeaks::processInputPeakTolerance() {
  // check code integrity
  if (m_numPeaksToFit == 0)
    throw std::runtime_error("ProcessInputPeakTolerance() must be called after "
                             "ProcessInputPeakCenters()");

  // peak tolerance
  m_peakPosTolerances = getProperty(PropertyNames::POSITION_TOL);

  if (m_peakPosTolerances.empty()) {
    // case 2, 3, 4
    m_peakPosTolerances.clear();
    m_peakPosTolCase234 = true;
  } else if (m_peakPosTolerances.size() == 1) {
    // only 1 uniform peak position tolerance is defined: expand to all peaks
    double peak_tol = m_peakPosTolerances[0];
    m_peakPosTolerances.resize(m_numPeaksToFit, peak_tol);
  } else if (m_peakPosTolerances.size() != m_numPeaksToFit) {
    // not uniform but number of peaks does not match
    g_log.error() << "number of peak position tolerance " << m_peakPosTolerances.size()
                  << " is not same as number of peaks " << m_numPeaksToFit << "\n";
    throw std::runtime_error("Number of peak position tolerances and number of "
                             "peaks to fit are inconsistent.");
  }

  // minimum peak height: set default to zero
  m_minPeakHeight = getProperty(PropertyNames::PEAK_MIN_HEIGHT);
  if (isEmpty(m_minPeakHeight) || m_minPeakHeight < 0.)
    m_minPeakHeight = 0.;

  return;
}

//----------------------------------------------------------------------------------------------
/** Convert the input initial parameter name/value to parameter index/value for
 * faster access
 * according to the parameter name and peak profile function
 * Output: m_initParamIndexes will be set up
 */
void FitPeaks::convertParametersNameToIndex() {
  // get a map for peak profile parameter name and parameter index
  std::map<std::string, size_t> parname_index_map;
  for (size_t iparam = 0; iparam < m_peakFunction->nParams(); ++iparam)
    parname_index_map.insert(std::make_pair(m_peakFunction->parameterName(iparam), iparam));

  // define peak parameter names (class variable) if using table
  if (m_profileStartingValueTable)
    m_peakParamNames = m_profileStartingValueTable->getColumnNames();

  // map the input parameter names to parameter indexes
  for (const auto &paramName : m_peakParamNames) {
    auto locator = parname_index_map.find(paramName);
    if (locator != parname_index_map.end()) {
      m_initParamIndexes.emplace_back(locator->second);
    } else {
      // a parameter name that is not defined in the peak profile function.  An
      // out-of-range index is thus set to this
      g_log.warning() << "Given peak parameter " << paramName
                      << " is not an allowed parameter of peak "
                         "function "
                      << m_peakFunction->name() << "\n";
      m_initParamIndexes.emplace_back(m_peakFunction->nParams() * 10);
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** main method to fit peaks among all
 */
std::vector<std::shared_ptr<FitPeaksAlgorithm::PeakFitResult>> FitPeaks::fitPeaks() {
  API::Progress prog(this, 0., 1., m_stopWorkspaceIndex - m_startWorkspaceIndex);

  /// Vector to record all the FitResult (only containing specified number of
  /// spectra. shift is expected)
  size_t num_fit_result = m_stopWorkspaceIndex - m_startWorkspaceIndex + 1;
  std::vector<std::shared_ptr<FitPeaksAlgorithm::PeakFitResult>> fit_result_vector(num_fit_result);

  const int nThreads = FrameworkManager::Instance().getNumOMPThreads();
  size_t chunkSize = num_fit_result / nThreads;

  PRAGMA_OMP(parallel for schedule(dynamic, 1) )
  for (int ithread = 0; ithread < nThreads; ithread++) {
    PARALLEL_START_INTERUPT_REGION
    auto iws_begin = m_startWorkspaceIndex + chunkSize * static_cast<size_t>(ithread);
    auto iws_end = (ithread == nThreads - 1) ? m_stopWorkspaceIndex + 1 : iws_begin + chunkSize;

    // vector to store fit params for last good fit to each peak
    std::vector<std::vector<double>> lastGoodPeakParameters(m_numPeaksToFit,
                                                            std::vector<double>(m_peakFunction->nParams(), 0.0));

    for (auto wi = iws_begin; wi < iws_end; ++wi) {
      // peaks to fit
      std::vector<double> expected_peak_centers = getExpectedPeakPositions(static_cast<size_t>(wi));

      // initialize output for this
      size_t numfuncparams = m_peakFunction->nParams() + m_bkgdFunction->nParams();
      std::shared_ptr<FitPeaksAlgorithm::PeakFitResult> fit_result =
          std::make_shared<FitPeaksAlgorithm::PeakFitResult>(m_numPeaksToFit, numfuncparams);

      fitSpectrumPeaks(static_cast<size_t>(wi), expected_peak_centers, fit_result, lastGoodPeakParameters);

      PARALLEL_CRITICAL(FindPeaks_WriteOutput) {
        writeFitResult(static_cast<size_t>(wi), expected_peak_centers, fit_result);
        fit_result_vector[wi - m_startWorkspaceIndex] = fit_result;
      }
      prog.report();
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  return fit_result_vector;
}

namespace {
/// Supported peak profiles for observation
std::vector<std::string> supported_peak_profiles{"Gaussian", "Lorentzian", "PseudoVoigt", "Voigt",
                                                 "BackToBackExponential"};

double numberCounts(const Histogram &histogram) {
  double total = 0.;
  for (const auto &value : histogram.y())
    total += std::fabs(value);
  return total;
}

//----------------------------------------------------------------------------------------------
/** Get number of counts in a specified range of a histogram
 * @param histogram :: histogram instance
 * @param xmin :: left boundary
 * @param xmax :: right boundary
 * @return :: counts
 */
double numberCounts(const Histogram &histogram, const double xmin, const double xmax) {
  const auto &vector_x = histogram.points();

  // determine left boundary
  auto start_iter = vector_x.begin();
  if (xmin > vector_x.front())
    start_iter = std::lower_bound(vector_x.begin(), vector_x.end(), xmin);
  if (start_iter == vector_x.end())
    return 0.; // past the end of the data means nothing to integrate
  // determine right boundary
  auto stop_iter = vector_x.end();
  if (xmax < vector_x.back()) // will set at end of vector if too large
    stop_iter = std::lower_bound(start_iter, stop_iter, xmax);

  // convert to indexes to sum over y
  size_t start_index = static_cast<size_t>(start_iter - vector_x.begin());
  size_t stop_index = static_cast<size_t>(stop_iter - vector_x.begin());

  // integrate
  double total = 0.;
  for (size_t i = start_index; i < stop_index; ++i)
    total += std::fabs(histogram.y()[i]);
  return total;
}
} // namespace

//----------------------------------------------------------------------------------------------
/** Fit peaks across one single spectrum
 */
void FitPeaks::fitSpectrumPeaks(size_t wi, const std::vector<double> &expected_peak_centers,
                                const std::shared_ptr<FitPeaksAlgorithm::PeakFitResult> &fit_result,
                                std::vector<std::vector<double>> &lastGoodPeakParameters) {
  // Spectrum contains very weak signal: do not proceed and return
  if (numberCounts(m_inputMatrixWS->histogram(wi)) <= m_minPeakHeight) {
    for (size_t i = 0; i < fit_result->getNumberPeaks(); ++i)
      fit_result->setBadRecord(i, -1.);
    return; // don't do anything
  }

  // Set up sub algorithm Fit for peak and background
  IAlgorithm_sptr peak_fitter; // both peak and background (combo)
  try {
    peak_fitter = createChildAlgorithm("Fit", -1, -1, false);
  } catch (Exception::NotFoundError &) {
    std::stringstream errss;
    errss << "The FitPeak algorithm requires the CurveFitting library";
    g_log.error(errss.str());
    throw std::runtime_error(errss.str());
  }

  // Clone background function
  IBackgroundFunction_sptr bkgdfunction = std::dynamic_pointer_cast<API::IBackgroundFunction>(m_bkgdFunction->clone());

  // set up properties of algorithm (reference) 'Fit'
  peak_fitter->setProperty("Minimizer", m_minimizer);
  peak_fitter->setProperty("CostFunction", m_costFunction);
  peak_fitter->setProperty("CalcErrors", true);

  const double x0 = m_inputMatrixWS->histogram(wi).x().front();
  const double xf = m_inputMatrixWS->histogram(wi).x().back();

  // index of previous peak in same spectrum (initially invalid)
  size_t prev_peak_index = m_numPeaksToFit;
  bool neighborPeakSameSpectrum = false;

  for (size_t fit_index = 0; fit_index < m_numPeaksToFit; ++fit_index) {
    // convert fit index to peak index (in ascending order)
    size_t peak_index(fit_index);
    if (m_fitPeaksFromRight)
      peak_index = m_numPeaksToFit - fit_index - 1;

    // reset the background function
    for (size_t i = 0; i < bkgdfunction->nParams(); ++i)
      bkgdfunction->setParameter(i, 0.);

    double expected_peak_pos = expected_peak_centers[peak_index];

    // clone peak function for each peak (need to do this so can
    // set center and calc any parameters from xml)
    auto peakfunction = std::dynamic_pointer_cast<API::IPeakFunction>(m_peakFunction->clone());
    peakfunction->setCentre(expected_peak_pos);
    peakfunction->setMatrixWorkspace(m_inputMatrixWS, wi, 0.0, 0.0);

    std::map<size_t, double> keep_values;
    for (size_t ipar = 0; ipar < peakfunction->nParams(); ++ipar) {
      if (peakfunction->isFixed(ipar)) {
        // save value of these parameters which have just been calculated
        // if they were set to be fixed (e.g. for the B2Bexp this would
        // typically be A and B but not Sigma)
        keep_values[ipar] = peakfunction->getParameter(ipar);
        // let them be free to fit as these are typically refined from a
        // focussed bank
        peakfunction->unfix(ipar);
      }
    }

    // Determine whether to set starting parameter from fitted value
    // of same peak but different spectrum
    bool samePeakCrossSpectrum = (lastGoodPeakParameters[peak_index].size() >
                                  static_cast<size_t>(std::count_if(lastGoodPeakParameters[peak_index].begin(),
                                                                    lastGoodPeakParameters[peak_index].end(),
                                                                    [&](auto const &val) { return val <= 1e-10; })));

    // Check whether current spectrum's pixel (detector ID) is close to its
    // previous spectrum's pixel (detector ID).
    try {
      if (wi > 0 && samePeakCrossSpectrum) {
        // First spectrum or discontinuous detector ID: do not start from same
        // peak of last spectrum
        std::shared_ptr<const Geometry::Detector> pdetector =
            std::dynamic_pointer_cast<const Geometry::Detector>(m_inputMatrixWS->getDetector(wi - 1));
        std::shared_ptr<const Geometry::Detector> cdetector =
            std::dynamic_pointer_cast<const Geometry::Detector>(m_inputMatrixWS->getDetector(wi));

        // If they do have detector ID
        if (pdetector && cdetector) {
          auto prev_id = pdetector->getID();
          auto curr_id = cdetector->getID();
          if (prev_id + 1 != curr_id)
            samePeakCrossSpectrum = false;
        } else {
          samePeakCrossSpectrum = false;
        }

      } else {
        // first spectrum in the workspace: no peak's fitting result to copy
        // from
        samePeakCrossSpectrum = false;
      }
    } catch (const std::runtime_error &) {
      // workspace does not have detector ID set: there is no guarantee that the
      // adjacent spectra can have similar peak profiles
      samePeakCrossSpectrum = false;
    }

    // Set starting values of the peak function
    if (samePeakCrossSpectrum) { // somePeakFit
      // Get from local best result
      for (size_t i = 0; i < peakfunction->nParams(); ++i) {
        peakfunction->setParameter(i, lastGoodPeakParameters[peak_index][i]);
      }
    } else if (neighborPeakSameSpectrum) {
      // set the peak parameters from last good fit to that peak
      for (size_t i = 0; i < peakfunction->nParams(); ++i) {
        peakfunction->setParameter(i, lastGoodPeakParameters[prev_peak_index][i]);
      }
    }

    // reset center though - don't know before hand which element this is
    peakfunction->setCentre(expected_peak_pos);
    // reset value of parameters that were fixed (but are now free to vary)
    for (const auto &[ipar, value] : keep_values) {
      peakfunction->setParameter(ipar, value);
    }

    double cost(DBL_MAX);
    if (expected_peak_pos <= x0 || expected_peak_pos >= xf) {
      // out of range and there won't be any fit
      peakfunction->setIntensity(0);
    } else {
      // find out the peak position to fit
      std::pair<double, double> peak_window_i = getPeakFitWindow(wi, peak_index);

      // Decide whether to estimate peak width by observation
      // If no peaks fitted in the same or cross spectrum then the user supplied
      // parameters will be used if present and the width will not be estimated
      // (note this will overwrite parameter values caluclated from
      // Parameters.xml)
      auto useUserSpecifedIfGiven = !(samePeakCrossSpectrum || neighborPeakSameSpectrum);
      bool observe_peak_width = decideToEstimatePeakParams(useUserSpecifedIfGiven, peakfunction);

      if (observe_peak_width && m_peakWidthEstimateApproach == EstimatePeakWidth::NoEstimation) {
        g_log.warning("Peak width can be estimated as ZERO.  The result can be wrong");
      }

      // do fitting with peak and background function (no analysis at this
      // point)
      cost = fitIndividualPeak(wi, peak_fitter, expected_peak_pos, peak_window_i, observe_peak_width, peakfunction,
                               bkgdfunction);
    }

    // process fitting result
    FitPeaksAlgorithm::FitFunction fit_function;
    fit_function.peakfunction = peakfunction;
    fit_function.bkgdfunction = bkgdfunction;

    auto good_fit = processSinglePeakFitResult(wi, peak_index, cost, expected_peak_centers, fit_function,
                                               fit_result); // sets the record

    if (good_fit) {
      // reset the flag such that there is at a peak fit in this spectrum
      neighborPeakSameSpectrum = true;
      prev_peak_index = peak_index;
      // copy values
      for (size_t i = 0; i < lastGoodPeakParameters[peak_index].size(); ++i) {
        lastGoodPeakParameters[peak_index][i] = peakfunction->getParameter(i);
      }
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Decide whether to estimate peak parameters. If not, then set the peak
 * parameters from
 * user specified starting value
 * @param firstPeakInSpectrum :: flag whether the given peak is the first peak
 * in the spectrum
 * @param peak_function :: peak function to set parameter values to
 * @return :: flag whether the peak width shall be observed
 */
bool FitPeaks::decideToEstimatePeakParams(const bool firstPeakInSpectrum,
                                          const API::IPeakFunction_sptr &peak_function) {
  // should observe the peak width if the user didn't supply all of the peak
  // function parameters
  bool observe_peak_shape(m_initParamIndexes.size() != peak_function->nParams());

  if (!m_initParamIndexes.empty()) {
    // user specifies starting value of peak parameters
    if (firstPeakInSpectrum) {
      // set the parameter values in a vector and loop over it
      // first peak.  using the user-specified value
      for (size_t i = 0; i < m_initParamIndexes.size(); ++i) {
        const size_t param_index = m_initParamIndexes[i];
        const double param_value = m_initParamValues[i];
        peak_function->setParameter(param_index, param_value);
      }
    } else {
      // using the fitted paramters from the previous fitting result
      // do noting
    }
  } else {
    // no previously defined peak parameters: observation is thus required
    observe_peak_shape = true;
  }

  return observe_peak_shape;
}

//----------------------------------------------------------------------------------------------
/** retrieve the fitted peak information from functions and set to output
 * vectors
 * @param wsindex :: workspace index
 * @param peakindex :: index of peak in given peak position vector
 * @param cost :: cost function value (i.e., chi^2)
 * @param expected_peak_positions :: vector of the expected peak positions
 * @param fitfunction :: pointer to function to retrieve information from
 * @param fit_result :: (output) PeakFitResult instance to set the fitting
 * result to
 * @return :: whether the peak fiting is good or not
 */
bool FitPeaks::processSinglePeakFitResult(size_t wsindex, size_t peakindex, const double cost,
                                          const std::vector<double> &expected_peak_positions,
                                          const FitPeaksAlgorithm::FitFunction &fitfunction,
                                          const std::shared_ptr<FitPeaksAlgorithm::PeakFitResult> &fit_result) {
  // determine peak position tolerance
  double postol(DBL_MAX);
  bool case23(false);
  if (m_peakPosTolCase234) {
    // peak tolerance is not defined
    if (m_numPeaksToFit == 1) {
      // case (d) one peak only
      postol = m_inputMatrixWS->histogram(wsindex).x().back() - m_inputMatrixWS->histogram(wsindex).x().front();
    } else {
      // case b and c: more than 1 peaks without defined peak tolerance
      case23 = true;
    }
  } else {
    // user explicitly specified
    if (peakindex >= m_peakPosTolerances.size())
      throw std::runtime_error("Peak tolerance out of index");
    postol = m_peakPosTolerances[peakindex];
  }

  // get peak position and analyze the fitting is good or not by various
  // criteria
  auto peak_pos = fitfunction.peakfunction->centre();
  auto peak_fwhm = fitfunction.peakfunction->fwhm();
  bool good_fit(false);
  if ((cost < 0) || (cost >= DBL_MAX - 1.) || std::isnan(cost)) {
    // unphysical cost function value
    peak_pos = -4;
  } else if (fitfunction.peakfunction->height() < m_minPeakHeight) {
    // peak height is under minimum request
    peak_pos = -3;
  } else if (case23) {
    // case b and c to check peak position without defined peak tolerance
    std::pair<double, double> fitwindow = getPeakFitWindow(wsindex, peakindex);
    if (fitwindow.first < fitwindow.second) {
      // peak fit window is specified or calculated: use peak window as position
      // tolerance
      if (peak_pos < fitwindow.first || peak_pos > fitwindow.second) {
        // peak is out of fit window
        peak_pos = -2;
        g_log.debug() << "Peak position " << peak_pos << " is out of fit "
                      << "window boundary " << fitwindow.first << ", " << fitwindow.second << "\n";
      } else if (peak_fwhm > (fitwindow.second - fitwindow.first)) {
        // peak is too wide or window is too small
        peak_pos = -2.25;
        g_log.debug() << "Peak position " << peak_pos << " has fwhm "
                      << "wider than the fit window " << fitwindow.second - fitwindow.first << "\n";
      } else {
        good_fit = true;
      }
    } else {
      // use the 1/2 distance to neiboring peak without defined peak window
      double left_bound(-1);
      if (peakindex > 0)
        left_bound = 0.5 * (expected_peak_positions[peakindex] - expected_peak_positions[peakindex - 1]);
      double right_bound(-1);
      if (peakindex < m_numPeaksToFit - 1)
        right_bound = 0.5 * (expected_peak_positions[peakindex + 1] - expected_peak_positions[peakindex]);
      if (left_bound < 0)
        left_bound = right_bound;
      if (right_bound < left_bound)
        right_bound = left_bound;
      if (left_bound < 0 || right_bound < 0)
        throw std::runtime_error("Code logic error such that left or right "
                                 "boundary of peak position is negative.");
      if (peak_pos < left_bound || peak_pos > right_bound) {
        peak_pos = -2.5;
      } else if (peak_fwhm > (right_bound - left_bound)) {
        // peak is too wide or window is too small
        peak_pos = -2.75;
        g_log.debug() << "Peak position " << peak_pos << " has fwhm "
                      << "wider than the fit window " << right_bound - left_bound << "\n";
      } else {
        good_fit = true;
      }
    }
  } else if (fabs(fitfunction.peakfunction->centre() - expected_peak_positions[peakindex]) > postol) {
    // peak center is not within tolerance
    peak_pos = -5;
    g_log.debug() << "Peak position difference "
                  << fabs(fitfunction.peakfunction->centre() - expected_peak_positions[peakindex])
                  << " is out of range of tolerance: " << postol << "\n";
  } else {
    // all criteria are passed
    good_fit = true;
  }

  // set cost function to DBL_MAX if fitting is bad
  double adjust_cost(cost);
  if (!good_fit) {
    // set the cost function value to DBL_MAX
    adjust_cost = DBL_MAX;
  }

  // reset cost
  if (adjust_cost > DBL_MAX - 1) {
    fitfunction.peakfunction->setIntensity(0);
  }

  // chi2
  fit_result->setRecord(peakindex, adjust_cost, peak_pos, fitfunction);

  return good_fit;
}

//----------------------------------------------------------------------------------------------
/** calculate fitted peaks with background in the output workspace
 * The current version gets the peak parameters and background parameters from
 * fitted parameter
 * table
 */
void FitPeaks::calculateFittedPeaks(std::vector<std::shared_ptr<FitPeaksAlgorithm::PeakFitResult>> fit_results) {
  // check
  if (!m_fittedParamTable)
    throw std::runtime_error("No parameters");

  const size_t num_peakfunc_params = m_peakFunction->nParams();
  const size_t num_bkgdfunc_params = m_bkgdFunction->nParams();

  PARALLEL_FOR_IF(Kernel::threadSafe(*m_fittedPeakWS))
  for (auto iws = static_cast<int64_t>(m_startWorkspaceIndex); iws <= static_cast<int64_t>(m_stopWorkspaceIndex);
       ++iws) {
    PARALLEL_START_INTERUPT_REGION
    // get a copy of peak function and background function
    IPeakFunction_sptr peak_function = std::dynamic_pointer_cast<IPeakFunction>(m_peakFunction->clone());
    IBackgroundFunction_sptr bkgd_function = std::dynamic_pointer_cast<IBackgroundFunction>(m_bkgdFunction->clone());
    std::shared_ptr<FitPeaksAlgorithm::PeakFitResult> fit_result_i = fit_results[iws - m_startWorkspaceIndex];
    // FIXME - This is a just a pure check
    if (!fit_result_i)
      throw std::runtime_error("There is something wroing with PeakFitResult vector!");

    for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak) {
      // get and set the peak function parameters
      const double chi2 = fit_result_i->getCost(ipeak);
      if (chi2 > 10.e10)
        continue;

      for (size_t iparam = 0; iparam < num_peakfunc_params; ++iparam)
        peak_function->setParameter(iparam, fit_result_i->getParameterValue(ipeak, iparam));
      for (size_t iparam = 0; iparam < num_bkgdfunc_params; ++iparam)
        bkgd_function->setParameter(iparam, fit_result_i->getParameterValue(ipeak, num_peakfunc_params + iparam));
      // use domain and function to calcualte
      // get the range of start and stop to construct a function domain
      const auto &vec_x = m_fittedPeakWS->points(static_cast<size_t>(iws));
      std::pair<double, double> peakwindow = getPeakFitWindow(static_cast<size_t>(iws), ipeak);
      auto start_x_iter = std::lower_bound(vec_x.begin(), vec_x.end(), peakwindow.first);
      auto stop_x_iter = std::lower_bound(vec_x.begin(), vec_x.end(), peakwindow.second);

      if (start_x_iter == stop_x_iter)
        throw std::runtime_error("Range size is zero in calculateFittedPeaks");

      FunctionDomain1DVector domain(start_x_iter, stop_x_iter);
      FunctionValues values(domain);
      CompositeFunction_sptr comp_func = std::make_shared<API::CompositeFunction>();
      comp_func->addFunction(peak_function);
      comp_func->addFunction(bkgd_function);
      comp_func->function(domain, values);

      // copy over the values
      size_t istart = static_cast<size_t>(start_x_iter - vec_x.begin());
      size_t istop = static_cast<size_t>(stop_x_iter - vec_x.begin());
      for (size_t yindex = istart; yindex < istop; ++yindex) {
        m_fittedPeakWS->dataY(static_cast<size_t>(iws))[yindex] = values.getCalculated(yindex - istart);
      }
    } // END-FOR (ipeak)
    PARALLEL_END_INTERUPT_REGION
  } // END-FOR (iws)
  PARALLEL_CHECK_INTERUPT_REGION

  return;
}
namespace {
double estimateBackgroundParameters(const Histogram &histogram, const std::pair<size_t, size_t> &peak_window,
                                    const API::IBackgroundFunction_sptr &bkgd_function) {
  // for estimating background parameters
  // 0 = constant, 1 = linear
  const auto POLYNOMIAL_ORDER = std::min<size_t>(1, bkgd_function->nParams());

  if (peak_window.first >= peak_window.second)
    throw std::runtime_error("Invalid peak window");

  // reset the background function
  const auto nParams = bkgd_function->nParams();
  for (size_t i = 0; i < nParams; ++i)
    bkgd_function->setParameter(i, 0.);

  // 10 is a magic number that worked in a variety of situations
  const size_t iback_start = peak_window.first + 10;
  const size_t iback_stop = peak_window.second - 10;

  double chisq{DBL_MAX}; // how well the fit worked

  // use the simple way to find linear background
  // there aren't enough bins in the window to try to estimate so just leave the
  // estimate at zero
  if (iback_start < iback_stop) {
    double bkgd_a0{0.}; // will be fit
    double bkgd_a1{0.}; // may be fit
    double bkgd_a2{0.}; // will be ignored
    HistogramData::estimateBackground(POLYNOMIAL_ORDER, histogram, peak_window.first, peak_window.second, iback_start,
                                      iback_stop, bkgd_a0, bkgd_a1, bkgd_a2, chisq);
    // update the background function with the result
    bkgd_function->setParameter(0, bkgd_a0);
    if (nParams > 1)
      bkgd_function->setParameter(1, bkgd_a1);
    // quadratic term is always estimated to be zero
  }

  return chisq;
}
} // anonymous namespace

//----------------------------------------------------------------------------------------------
/** check whether a peak profile is allowed to observe peak width and set width
 * @brief isObservablePeakProfile
 * @param peakprofile : name of peak profile to check against
 * @return :: flag whether the specified peak profile observable
 */
bool FitPeaks::isObservablePeakProfile(const std::string &peakprofile) {
  return (std::find(supported_peak_profiles.begin(), supported_peak_profiles.end(), peakprofile) !=
          supported_peak_profiles.end());
}

//----------------------------------------------------------------------------------------------
/** Fit background function
 */
bool FitPeaks::fitBackground(const size_t &ws_index, const std::pair<double, double> &fit_window,
                             const double &expected_peak_pos, const API::IBackgroundFunction_sptr &bkgd_func) {
  constexpr size_t MIN_POINTS{10}; // TODO explain why 10

  // find out how to fit background
  const auto &points = m_inputMatrixWS->histogram(ws_index).points();
  size_t start_index = findXIndex(points.rawData(), fit_window.first);
  size_t expected_peak_index = findXIndex(points.rawData(), expected_peak_pos, start_index);
  size_t stop_index = findXIndex(points.rawData(), fit_window.second, expected_peak_index);

  // treat 5 as a magic number - TODO explain why
  bool good_fit(false);
  if (expected_peak_index - start_index > MIN_POINTS && stop_index - expected_peak_index > MIN_POINTS) {
    // enough data points left for multi-domain fitting
    // set a smaller fit window
    const std::pair<double, double> vec_min{fit_window.first, points[expected_peak_index + 5]};
    const std::pair<double, double> vec_max{points[expected_peak_index - 5], fit_window.second};

    // reset background function value
    for (size_t n = 0; n < bkgd_func->nParams(); ++n)
      bkgd_func->setParameter(n, 0);

    double chi2 = fitFunctionMD(bkgd_func, m_inputMatrixWS, ws_index, vec_min, vec_max);

    // process
    if (chi2 < DBL_MAX - 1) {
      good_fit = true;
    }

  } else {
    // fit as a single domain function.  check whether the result is good or bad

    // TODO FROM HERE!
    g_log.debug() << "Don't know what to do with background fitting with single "
                  << "domain function! " << (expected_peak_index - start_index) << " points to the left "
                  << (stop_index - expected_peak_index) << " points to the right\n";
  }

  return good_fit;
}

//----------------------------------------------------------------------------------------------
/** Fit an individual peak
 */
double FitPeaks::fitIndividualPeak(size_t wi, const API::IAlgorithm_sptr &fitter, const double expected_peak_center,
                                   const std::pair<double, double> &fitwindow, const bool estimate_peak_width,
                                   const API::IPeakFunction_sptr &peakfunction,
                                   const API::IBackgroundFunction_sptr &bkgdfunc) {
  double cost(DBL_MAX);

  // confirm that there is something to fit
  if (numberCounts(m_inputMatrixWS->histogram(wi), fitwindow.first, fitwindow.second) <= m_minPeakHeight)
    return cost;

  if (m_highBackground) {
    // fit peak with high background!
    cost = fitFunctionHighBackground(fitter, fitwindow, wi, expected_peak_center, estimate_peak_width, peakfunction,
                                     bkgdfunc);
  } else {
    // fit peak and background
    cost = fitFunctionSD(fitter, peakfunction, bkgdfunc, m_inputMatrixWS, wi, fitwindow, expected_peak_center,
                         estimate_peak_width, true);
  }

  return cost;
}

//----------------------------------------------------------------------------------------------
/** Fit function in single domain (mostly applied for fitting peak + background)
 * with estimating peak parameters
 * This is the core fitting algorithm to deal with the simplest situation
 * @exception :: Fit.isExecuted is false (cannot be executed)
 */
double FitPeaks::fitFunctionSD(const IAlgorithm_sptr &fit, const API::IPeakFunction_sptr &peak_function,
                               const API::IBackgroundFunction_sptr &bkgd_function,
                               const API::MatrixWorkspace_sptr &dataws, size_t wsindex,
                               const std::pair<double, double> &peak_range, const double &expected_peak_center,
                               bool estimate_peak_width, bool estimate_background) {
  std::stringstream errorid;
  errorid << "(WorkspaceIndex=" << wsindex << " PeakCentre=" << expected_peak_center << ")";

  // generate peak window
  if (peak_range.first >= peak_range.second) {
    std::stringstream msg;
    msg << "Invalid peak window: xmin>xmax (" << peak_range.first << ", " << peak_range.second << ")" << errorid.str();
    throw std::runtime_error(msg.str());
  }

  // determine the peak window in array index
  const auto &histogram = dataws->histogram(wsindex);
  const auto &vector_x = histogram.points();
  const auto start_index = findXIndex(vector_x, peak_range.first);
  const auto stop_index = findXIndex(vector_x, peak_range.second, start_index);
  if (start_index == stop_index)
    throw std::runtime_error("Range size is zero in estimatePeakParameters");
  std::pair<size_t, size_t> peak_window = std::make_pair(start_index, stop_index);

  // Estimate background
  if (estimate_background) {
    estimateBackgroundParameters(histogram, peak_window, bkgd_function);
  }

  // Estimate peak profile parameter
  peak_function->setCentre(expected_peak_center); // set expected position first
  int result = estimatePeakParameters(histogram, peak_window, peak_function, bkgd_function, estimate_peak_width,
                                      m_peakWidthEstimateApproach, m_peakWidthPercentage, m_minPeakHeight);
  if (result != GOOD) {
    peak_function->setCentre(expected_peak_center);
    if (result == NOSIGNAL || result == LOWPEAK) {
      return DBL_MAX; // exit early - don't fit
    }
  }

  // Create the composition function
  CompositeFunction_sptr comp_func = std::make_shared<API::CompositeFunction>();
  comp_func->addFunction(peak_function);
  comp_func->addFunction(bkgd_function);
  IFunction_sptr fitfunc = std::dynamic_pointer_cast<IFunction>(comp_func);

  // Set the properties
  fit->setProperty("Function", fitfunc);
  fit->setProperty("InputWorkspace", dataws);
  fit->setProperty("WorkspaceIndex", static_cast<int>(wsindex));
  fit->setProperty("MaxIterations", m_fitIterations); // magic number
  fit->setProperty("StartX", peak_range.first);
  fit->setProperty("EndX", peak_range.second);
  fit->setProperty("IgnoreInvalidData", true);

  if (m_constrainPeaksPosition) {
    // set up a constraint on peak position
    double peak_center = peak_function->centre();
    double peak_width = peak_function->fwhm();
    std::stringstream peak_center_constraint;
    peak_center_constraint << (peak_center - 0.5 * peak_width) << " < f0." << peak_function->getCentreParameterName()
                           << " < " << (peak_center + 0.5 * peak_width);

    // set up a constraint on peak height
    fit->setProperty("Constraints", peak_center_constraint.str());
  }

  // Execute fit and get result of fitting background
  g_log.debug() << "[E1201] FitSingleDomain Before fitting, Fit function: " << fit->asString() << "\n";
  errorid << " starting function [" << comp_func->asString() << "]";
  try {
    fit->execute();
    g_log.debug() << "[E1202] FitSingleDomain After fitting, Fit function: " << fit->asString() << "\n";

    if (!fit->isExecuted()) {
      g_log.warning() << "Fitting peak SD (single domain) failed to execute. " + errorid.str();
      return DBL_MAX;
    }
  } catch (std::invalid_argument &e) {
    errorid << ": " << e.what();
    g_log.warning() << "While fitting " + errorid.str();
    return DBL_MAX; // probably the wrong thing to do
  }

  // Retrieve result
  std::string fitStatus = fit->getProperty("OutputStatus");
  double chi2{std::numeric_limits<double>::max()};
  if (fitStatus == "success") {
    chi2 = fit->getProperty("OutputChi2overDoF");
  }

  return chi2;
}

//----------------------------------------------------------------------------------------------
double FitPeaks::fitFunctionMD(API::IFunction_sptr fit_function, const API::MatrixWorkspace_sptr &dataws,
                               const size_t wsindex, const std::pair<double, double> &vec_xmin,
                               const std::pair<double, double> &vec_xmax) {
  // Note: after testing it is found that multi-domain Fit cannot be reused
  API::IAlgorithm_sptr fit;
  try {
    fit = createChildAlgorithm("Fit", -1, -1, false);
  } catch (Exception::NotFoundError &) {
    std::stringstream errss;
    errss << "The FitPeak algorithm requires the CurveFitting library";
    throw std::runtime_error(errss.str());
  }
  // set up background fit instance
  fit->setProperty("Minimizer", m_minimizer);
  fit->setProperty("CostFunction", m_costFunction);
  fit->setProperty("CalcErrors", true);

  // This use multi-domain; but does not know how to set up IFunction_sptr
  // fitfunc,
  std::shared_ptr<MultiDomainFunction> md_function = std::make_shared<MultiDomainFunction>();

  // Set function first
  md_function->addFunction(std::move(fit_function));

  //  set domain for function with index 0 covering both sides
  md_function->clearDomainIndices();
  md_function->setDomainIndices(0, {0, 1});

  // Set the properties
  fit->setProperty("Function", std::dynamic_pointer_cast<IFunction>(md_function));
  fit->setProperty("InputWorkspace", dataws);
  fit->setProperty("WorkspaceIndex", static_cast<int>(wsindex));
  fit->setProperty("StartX", vec_xmin.first);
  fit->setProperty("EndX", vec_xmax.first);
  fit->setProperty("InputWorkspace_1", dataws);
  fit->setProperty("WorkspaceIndex_1", static_cast<int>(wsindex));
  fit->setProperty("StartX_1", vec_xmin.second);
  fit->setProperty("EndX_1", vec_xmax.second);
  fit->setProperty("MaxIterations", m_fitIterations);
  fit->setProperty("IgnoreInvalidData", true);

  // Execute
  fit->execute();
  if (!fit->isExecuted()) {
    throw runtime_error("Fit is not executed on multi-domain function/data. ");
  }

  // Retrieve result
  std::string fitStatus = fit->getProperty("OutputStatus");

  double chi2 = DBL_MAX;
  if (fitStatus == "success") {
    chi2 = fit->getProperty("OutputChi2overDoF");
  }

  return chi2;
}

//----------------------------------------------------------------------------------------------
/// Fit peak with high background
double FitPeaks::fitFunctionHighBackground(const IAlgorithm_sptr &fit, const std::pair<double, double> &fit_window,
                                           const size_t &ws_index, const double &expected_peak_center,
                                           bool observe_peak_shape, const API::IPeakFunction_sptr &peakfunction,
                                           const API::IBackgroundFunction_sptr &bkgdfunc) {
  // high background to reduce
  API::IBackgroundFunction_sptr high_bkgd_function(nullptr);
  if (m_linearBackgroundFunction)
    high_bkgd_function = std::dynamic_pointer_cast<API::IBackgroundFunction>(m_linearBackgroundFunction->clone());

  // Fit the background first if there is enough data points
  fitBackground(ws_index, fit_window, expected_peak_center, high_bkgd_function);

  // Get partial of the data
  std::vector<double> vec_x, vec_y, vec_e;
  getRangeData(ws_index, fit_window, vec_x, vec_y, vec_e);

  // Reduce the background
  reduceByBackground(high_bkgd_function, vec_x, vec_y);
  for (size_t n = 0; n < bkgdfunc->nParams(); ++n)
    bkgdfunc->setParameter(n, 0);

  // Create a new workspace
  API::MatrixWorkspace_sptr reduced_bkgd_ws = createMatrixWorkspace(vec_x, vec_y, vec_e);

  // Fit peak with background
  fitFunctionSD(fit, peakfunction, bkgdfunc, reduced_bkgd_ws, 0, {vec_x.front(), vec_x.back()}, expected_peak_center,
                observe_peak_shape, false);

  // add the reduced background back
  bkgdfunc->setParameter(0, bkgdfunc->getParameter(0) + high_bkgd_function->getParameter(0));
  bkgdfunc->setParameter(1, bkgdfunc->getParameter(1) + // TODO doesn't work for flat background
                                high_bkgd_function->getParameter(1));

  double cost = fitFunctionSD(fit, peakfunction, bkgdfunc, m_inputMatrixWS, ws_index, {vec_x.front(), vec_x.back()},
                              expected_peak_center, false, false);

  return cost;
}

//----------------------------------------------------------------------------------------------
/// Create a single spectrum workspace for fitting
API::MatrixWorkspace_sptr FitPeaks::createMatrixWorkspace(const std::vector<double> &vec_x,
                                                          const std::vector<double> &vec_y,
                                                          const std::vector<double> &vec_e) {
  size_t size = vec_x.size();
  size_t ysize = vec_y.size();

  HistogramBuilder builder;
  builder.setX(size);
  builder.setY(ysize);
  MatrixWorkspace_sptr matrix_ws = create<Workspace2D>(1, builder.build());

  auto &dataX = matrix_ws->mutableX(0);
  auto &dataY = matrix_ws->mutableY(0);
  auto &dataE = matrix_ws->mutableE(0);

  dataX.assign(vec_x.cbegin(), vec_x.cend());
  dataY.assign(vec_y.cbegin(), vec_y.cend());
  dataE.assign(vec_e.cbegin(), vec_e.cend());

  return matrix_ws;
}

//----------------------------------------------------------------------------------------------
/** generate output workspace for peak positions
 */
void FitPeaks::generateOutputPeakPositionWS() {
  // create output workspace for peak positions: can be partial spectra to input
  // workspace
  size_t num_hist = m_stopWorkspaceIndex - m_startWorkspaceIndex + 1;
  m_outputPeakPositionWorkspace = create<Workspace2D>(num_hist, Points(m_numPeaksToFit));
  // set default
  for (size_t wi = 0; wi < num_hist; ++wi) {
    // convert to workspace index of input data workspace
    size_t inp_wi = wi + m_startWorkspaceIndex;
    std::vector<double> expected_position = getExpectedPeakPositions(inp_wi);
    for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak) {
      m_outputPeakPositionWorkspace->dataX(wi)[ipeak] = expected_position[ipeak];
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Set up parameter table (parameter value or error)
 * @brief FitPeaks::generateParameterTable
 * @param table_ws:: an empty workspace
 * @param param_names
 * @param with_chi2:: flag to append chi^2 to the table
 */
void FitPeaks::setupParameterTableWorkspace(const API::ITableWorkspace_sptr &table_ws,
                                            const std::vector<std::string> &param_names, bool with_chi2) {

  // add columns
  table_ws->addColumn("int", "wsindex");
  table_ws->addColumn("int", "peakindex");
  for (const auto &param_name : param_names)
    table_ws->addColumn("double", param_name);
  if (with_chi2)
    table_ws->addColumn("double", "chi2");

  // add rows
  const size_t numParam = m_fittedParamTable->columnCount() - 3;
  for (size_t iws = m_startWorkspaceIndex; iws <= m_stopWorkspaceIndex; ++iws) {
    for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak) {
      API::TableRow newRow = table_ws->appendRow();
      newRow << static_cast<int>(iws);   // workspace index
      newRow << static_cast<int>(ipeak); // peak number
      for (size_t iparam = 0; iparam < numParam; ++iparam)
        newRow << 0.; // parameters for each peak
      if (with_chi2)
        newRow << DBL_MAX; // chisq
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Generate table workspace for fitted parameters' value
 * and optionally the table workspace for those parameters' fitting error
 * @brief FitPeaks::generateFittedParametersValueWorkspace
 */
void FitPeaks::generateFittedParametersValueWorkspaces() {
  // peak parameter workspace
  m_rawPeaksTable = getProperty(PropertyNames::RAW_PARAMS);

  // create parameters
  // peak
  std::vector<std::string> param_vec;
  if (m_rawPeaksTable) {
    std::vector<std::string> peak_params = m_peakFunction->getParameterNames();
    for (const auto &peak_param : peak_params)
      param_vec.emplace_back(peak_param);
  } else {
    param_vec.emplace_back("centre");
    param_vec.emplace_back("width");
    param_vec.emplace_back("height");
    param_vec.emplace_back("intensity");
  }
  // background
  for (size_t iparam = 0; iparam < m_bkgdFunction->nParams(); ++iparam)
    param_vec.emplace_back(m_bkgdFunction->parameterName(iparam));

  // parameter value table
  m_fittedParamTable = std::make_shared<TableWorkspace>();
  setupParameterTableWorkspace(m_fittedParamTable, param_vec, true);

  // for error workspace
  std::string fiterror_table_name = getPropertyValue(PropertyNames::OUTPUT_WKSP_PARAM_ERRS);
  // do nothing if user does not specifiy
  if (fiterror_table_name.empty()) {
    // not specified
    m_fitErrorTable = nullptr;
  } else {
    // create table and set up parameter table
    m_fitErrorTable = std::make_shared<TableWorkspace>();
    setupParameterTableWorkspace(m_fitErrorTable, param_vec, false);
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Generate the output MatrixWorkspace for calculated peaks (as an option)
 * @brief FitPeaks::generateCalculatedPeaksWS
 */
void FitPeaks::generateCalculatedPeaksWS() {
  // matrix workspace contained calculated peaks from fitting
  std::string fit_ws_name = getPropertyValue(PropertyNames::OUTPUT_WKSP_MODEL);
  if (fit_ws_name.size() == 0) {
    // skip if user does not specify
    m_fittedPeakWS = nullptr;
    return;
  }

  // create a wokspace with same size as in the input matrix workspace
  m_fittedPeakWS = create<Workspace2D>(*m_inputMatrixWS);
}

//----------------------------------------------------------------------------------------------
/// set up output workspaces
void FitPeaks::processOutputs(std::vector<std::shared_ptr<FitPeaksAlgorithm::PeakFitResult>> fit_result_vec) {
  setProperty(PropertyNames::OUTPUT_WKSP, m_outputPeakPositionWorkspace);
  setProperty(PropertyNames::OUTPUT_WKSP_PARAMS, m_fittedParamTable);

  if (m_fitErrorTable) {
    g_log.warning("Output error table workspace");
    setProperty(PropertyNames::OUTPUT_WKSP_PARAM_ERRS, m_fitErrorTable);
  } else {
    g_log.warning("No error table output");
  }

  // optional
  if (m_fittedPeakWS && m_fittedParamTable) {
    g_log.debug("about to calcualte fitted peaks");
    calculateFittedPeaks(std::move(fit_result_vec));
    setProperty(PropertyNames::OUTPUT_WKSP_MODEL, m_fittedPeakWS);
  }
}

//----------------------------------------------------------------------------------------------
/// Get the expected peak's position
std::vector<double> FitPeaks::getExpectedPeakPositions(size_t wi) {
  // check
  if (wi < m_startWorkspaceIndex || wi > m_stopWorkspaceIndex) {
    std::stringstream errss;
    errss << "Workspace index " << wi << " is out of range [" << m_startWorkspaceIndex << ", " << m_stopWorkspaceIndex
          << "]";
    throw std::runtime_error(errss.str());
  }

  // initialize output array
  std::vector<double> exp_centers(m_numPeaksToFit);

  if (m_uniformPeakPositions) {
    // uniform peak centers among spectra: simple copy
    exp_centers = m_peakCenters;
  } else {
    // no uniform peak center.  locate the input workspace index
    // in the peak center workspace peak in the workspae

    // get the relative workspace index in input peak position workspace
    size_t peak_wi = wi - m_startWorkspaceIndex;
    // get values
    exp_centers = m_peakCenterWorkspace->x(peak_wi).rawData();
  }

  return exp_centers;
}

//----------------------------------------------------------------------------------------------
/// get the peak fit window
std::pair<double, double> FitPeaks::getPeakFitWindow(size_t wi, size_t ipeak) {
  // check workspace index
  if (wi < m_startWorkspaceIndex || wi > m_stopWorkspaceIndex) {
    std::stringstream errss;
    errss << "Workspace index " << wi << " is out of range [" << m_startWorkspaceIndex << ", " << m_stopWorkspaceIndex
          << "]";
    throw std::runtime_error(errss.str());
  }

  // check peak index
  if (ipeak >= m_numPeaksToFit) {
    std::stringstream errss;
    errss << "Peak index " << ipeak << " is out of range (" << m_numPeaksToFit << ")";
    throw std::runtime_error(errss.str());
  }

  double left(0), right(0);
  if (m_calculateWindowInstrument) {
    // calcualte peak window by delta(d)/d
    double peak_pos = getExpectedPeakPositions(wi)[ipeak];
    // calcalate expected peak width
    double estimate_peak_width = peak_pos * m_peakWidthPercentage;
    // using a MAGIC number to estimate the peak window
    double MAGIC = 3.0;
    left = peak_pos - estimate_peak_width * MAGIC;
    right = peak_pos + estimate_peak_width * MAGIC;
  } else if (m_uniformPeakWindows) {
    // uniform peak fit window
    assert(m_peakWindowVector.size() > 0); // peak fit window must be given!

    left = m_peakWindowVector[ipeak][0];
    right = m_peakWindowVector[ipeak][1];
  } else if (m_peakWindowWorkspace) {
    // no uniform peak fit window.  locate peak in the workspace
    // get workspace index in m_peakWindowWorkspace
    size_t window_wi = wi - m_startWorkspaceIndex;

    left = m_peakWindowWorkspace->x(window_wi)[ipeak * 2];
    right = m_peakWindowWorkspace->x(window_wi)[ipeak * 2 + 1];
  } else {
    throw std::runtime_error("Unhandled case for get peak fit window!");
  }
  if (left >= right) {
    std::stringstream errss;
    errss << "Peak window is inappropriate for workspace index " << wi << " peak " << ipeak << ": " << left
          << " >= " << right;
    throw std::runtime_error(errss.str());
  }

  return std::make_pair(left, right);
}

//----------------------------------------------------------------------------------------------
/** get vector X, Y and E in a given range
 */
void FitPeaks::getRangeData(size_t iws, const std::pair<double, double> &fit_window, std::vector<double> &vec_x,
                            std::vector<double> &vec_y, std::vector<double> &vec_e) {

  // get the original vector of X and determine the start and end index
  const auto &orig_x = m_inputMatrixWS->histogram(iws).x();
  size_t left_index = findXIndex(orig_x, fit_window.first);
  size_t right_index = findXIndex(orig_x, fit_window.second, left_index);

  if (left_index >= right_index) {
    std::stringstream err_ss;
    err_ss << "Unable to get subset of histogram from given fit window. "
           << "Fit window: " << fit_window.first << ", " << fit_window.second << ". Vector X's range is "
           << orig_x.front() << ", " << orig_x.back();
    throw std::runtime_error(err_ss.str());
  }

  // copy X, Y and E
  size_t num_elements = right_index - left_index;
  vec_x.resize(num_elements);
  std::copy(orig_x.begin() + left_index, orig_x.begin() + right_index, vec_x.begin());

  // modify right_index if it is at the end
  if (m_inputMatrixWS->isHistogramData() && right_index == orig_x.size() - 1) {
    right_index -= 1;
    if (right_index == left_index)
      throw std::runtime_error("Histogram workspace have same left and right "
                               "boundary index for Y and E.");
    num_elements -= 1;
  }

  // get original vector of Y and E
  const std::vector<double> orig_y = m_inputMatrixWS->histogram(iws).y().rawData();
  const std::vector<double> orig_e = m_inputMatrixWS->histogram(iws).e().rawData();
  vec_y.resize(num_elements);
  vec_e.resize(num_elements);
  std::copy(orig_y.begin() + left_index, orig_y.begin() + right_index, vec_y.begin());
  std::copy(orig_e.begin() + left_index, orig_e.begin() + right_index, vec_e.begin());

  return;
}

//----------------------------------------------------------------------------------------------
/** Reduce Y value with given background function
 * @param bkgd_func :: bacground function pointer
 * @param vec_x :: vector of X valye
 * @param vec_y :: (input/output) vector Y to be reduced by background function
 */
void FitPeaks::reduceByBackground(const API::IBackgroundFunction_sptr &bkgd_func, const std::vector<double> &vec_x,
                                  std::vector<double> &vec_y) {
  // calculate the background
  FunctionDomain1DVector vectorx(vec_x.begin(), vec_x.end());
  FunctionValues vector_bkgd(vectorx);
  bkgd_func->function(vectorx, vector_bkgd);

  // subtract the background from the supplied data
  for (size_t i = 0; i < vec_y.size(); ++i) {
    (vec_y)[i] -= vector_bkgd[i];
    // it is better not to mess up with E here
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Write result of peak fit per spectrum to output analysis workspaces
 * including (1) output peak position workspace (2) parameter table workspace
 * and optionally (3) fitting error/uncertainty workspace
 * @brief FitPeaks::writeFitResult
 * @param wi
 * @param expected_positions :: vector for expected peak positions
 * @param fit_result :: PeakFitResult instance
 */
void FitPeaks::writeFitResult(size_t wi, const std::vector<double> &expected_positions,
                              const std::shared_ptr<FitPeaksAlgorithm::PeakFitResult> &fit_result) {
  // convert to
  size_t out_wi = wi - m_startWorkspaceIndex;
  if (out_wi >= m_outputPeakPositionWorkspace->getNumberHistograms()) {
    g_log.error() << "workspace index " << wi << " is out of output peak position workspace "
                  << "range of spectra, which contains " << m_outputPeakPositionWorkspace->getNumberHistograms()
                  << " spectra"
                  << "\n";
    throw std::runtime_error("Out of boundary to set output peak position workspace");
  }

  // Fill the output peak position workspace
  for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak) {
    double exp_peak_pos(expected_positions[ipeak]);
    double fitted_peak_pos = fit_result->getPeakPosition(ipeak);
    double peak_chi2 = fit_result->getCost(ipeak);

    m_outputPeakPositionWorkspace->mutableX(out_wi)[ipeak] = exp_peak_pos;
    m_outputPeakPositionWorkspace->mutableY(out_wi)[ipeak] = fitted_peak_pos;
    m_outputPeakPositionWorkspace->mutableE(out_wi)[ipeak] = peak_chi2;
  }

  // Output the peak parameters to the table workspace
  // check vector size

  // last column of the table is for chi2
  size_t chi2_index = m_fittedParamTable->columnCount() - 1;

  // check TableWorkspace and given FitResult
  if (m_rawPeaksTable) {
    // duplicate from FitPeakResult to table workspace
    // check again with the column size versus peak parameter values
    if (fit_result->getNumberParameters() != m_fittedParamTable->columnCount() - 3) {
      g_log.error() << "Peak of type (" << m_peakFunction->name() << ") has " << fit_result->getNumberParameters()
                    << " parameters.  Parameter table shall have 3 more "
                       "columns.  But not it has "
                    << m_fittedParamTable->columnCount() << " columns\n";
      throw std::runtime_error("Peak parameter vector for one peak has different sizes to output "
                               "table workspace");
    }
  } else {
    // effective peak profile parameters: need to re-construct the peak function
    if (4 + m_bkgdFunction->nParams() != m_fittedParamTable->columnCount() - 3) {

      std::stringstream err_ss;
      err_ss << "Peak has 4 effective peak parameters and " << m_bkgdFunction->nParams() << " background parameters "
             << ". Parameter table shall have 3 more  columns.  But not it has " << m_fittedParamTable->columnCount()
             << " columns";
      throw std::runtime_error(err_ss.str());
    }
  }

  // go through each peak
  // get a copy of peak function and background function
  IPeakFunction_sptr peak_function = std::dynamic_pointer_cast<IPeakFunction>(m_peakFunction->clone());
  size_t num_peakfunc_params = peak_function->nParams();
  size_t num_bkgd_params = m_bkgdFunction->nParams();

  for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak) {
    // get row number
    size_t row_index = out_wi * m_numPeaksToFit + ipeak;

    // treat as different cases for writing out raw or effective parametr
    if (m_rawPeaksTable) {
      // duplicate from FitPeakResult to table workspace
      for (size_t iparam = 0; iparam < num_peakfunc_params + num_bkgd_params; ++iparam) {
        size_t col_index = iparam + 2;
        // fitted parameter's value
        m_fittedParamTable->cell<double>(row_index, col_index) = fit_result->getParameterValue(ipeak, iparam);
        // fitted parameter's fitting error
        if (m_fitErrorTable) {
          m_fitErrorTable->cell<double>(row_index, col_index) = fit_result->getParameterError(ipeak, iparam);
        }

      } // end for (iparam)
    } else {
      // effective peak profile parameter
      // construct the peak function
      for (size_t iparam = 0; iparam < num_peakfunc_params; ++iparam)
        peak_function->setParameter(iparam, fit_result->getParameterValue(ipeak, iparam));

      // set the effective peak parameters
      m_fittedParamTable->cell<double>(row_index, 2) = peak_function->centre();
      m_fittedParamTable->cell<double>(row_index, 3) = peak_function->fwhm();
      m_fittedParamTable->cell<double>(row_index, 4) = peak_function->height();
      m_fittedParamTable->cell<double>(row_index, 5) = peak_function->intensity();

      // background
      for (size_t iparam = 0; iparam < num_bkgd_params; ++iparam)
        m_fittedParamTable->cell<double>(row_index, 6 + iparam) =
            fit_result->getParameterValue(ipeak, num_peakfunc_params + iparam);
    }

    // set chi2
    m_fittedParamTable->cell<double>(row_index, chi2_index) = fit_result->getCost(ipeak);
  }

  return;
}

//----------------------------------------------------------------------------------------------
std::string FitPeaks::getPeakHeightParameterName(const API::IPeakFunction_const_sptr &peak_function) {
  std::string height_name("");

  std::vector<std::string> peak_parameters = peak_function->getParameterNames();
  for (const auto &name : peak_parameters) {
    if (name == "Height") {
      height_name = "Height";
      break;
    } else if (name == "I") {
      height_name = "I";
      break;
    } else if (name == "Intensity") {
      height_name = "Intensity";
      break;
    }
  }

  if (height_name.empty())
    throw std::runtime_error("Peak height parameter name cannot be found.");

  return height_name;
}

DECLARE_ALGORITHM(FitPeaks)

} // namespace Mantid::Algorithms
