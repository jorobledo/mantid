// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FunctionBrowser.h"
#include "MantidQtWidgets/Common/FunctionMultiDomainPresenter.h"
#include "MantidQtWidgets/Common/FunctionTreeView.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/ParameterTie.h"

#include "MantidKernel/Logger.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QMessageBox>

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <utility>

namespace {
Mantid::Kernel::Logger g_log("Function Browser");
} // namespace

namespace MantidQt::MantidWidgets {

using namespace Mantid::API;
using namespace Mantid::Kernel;

/**
 * Constructor
 * @param parent :: The parent widget.
 * @param multi  :: Option to use the browser for multi-dataset fitting.
 * @param categories :: Function categories to be included to the Add Function
 * dialog. An empty vector means include all available categories.
 */
FunctionBrowser::FunctionBrowser(QWidget *parent, bool multi, const std::vector<std::string> &categories)
    : QWidget(parent) {
  auto view = new FunctionTreeView(this, multi, categories);
  m_presenter = std::make_unique<FunctionMultiDomainPresenter>(view);
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setMargin(0);
  layout->addWidget(view);
  connect(m_presenter.get(), SIGNAL(functionStructureChanged()), this, SIGNAL(functionStructureChanged()));
  connect(m_presenter.get(), SIGNAL(parameterChanged(const QString &, const QString &)), this,
          SIGNAL(parameterChanged(const QString &, const QString &)));
  connect(m_presenter.get(), SIGNAL(attributeChanged(const QString &)), this,
          SIGNAL(attributeChanged(const QString &)));
}

/**
 * Destructor
 */
FunctionBrowser::~FunctionBrowser() {}

/**
 * Clear the contents
 */
void FunctionBrowser::clear() { m_presenter->clear(); }

/**
 * Set the function in the browser
 * @param funStr :: FunctionFactory function creation string
 */
void FunctionBrowser::setFunction(const QString &funStr) { m_presenter->setFunctionString(funStr); }

/**
 * Set the function in the browser
 * @param fun :: A function
 */
void FunctionBrowser::setFunction(IFunction_sptr fun) { m_presenter->setFunction(std::move(fun)); }

/**
 * Return function at specified function index (e.g. f0.)
 * @param index :: Index of the function, or empty string for top-level function
 * @return Function at index, or null pointer if not found
 */
IFunction_sptr FunctionBrowser::getFunctionByIndex(const QString &index) {
  return m_presenter->getFunctionByIndex(index);
}
/**
 * Updates the function parameter value
 * @param paramName :: Fully qualified parameter name (includes function index)
 * @param value :: New value
 */
void FunctionBrowser::setParameter(const QString &paramName, double value) {
  m_presenter->setParameter(paramName, value);
}

/**
 * Updates the function parameter error
 * @param paramName :: Fully qualified parameter name (includes function index)
 * @param error :: New error
 */
void FunctionBrowser::setParameterError(const QString &paramName, double error) {
  m_presenter->setParameterError(paramName, error);
}

/**
 * Get a value of a parameter
 * @param paramName :: Fully qualified parameter name (includes function index)
 */
double FunctionBrowser::getParameter(const QString &paramName) const { return m_presenter->getParameter(paramName); }

/**
 * Update parameter values in the browser to match those of a function.
 * @param fun :: A function to copy the values from. It must have the same
 *   type (composition) as the function in the browser.
 */
void FunctionBrowser::updateParameters(const IFunction &fun) { m_presenter->updateParameters(fun); }

/**
 * Return FunctionFactory function string
 */
QString FunctionBrowser::getFunctionString() { return m_presenter->getFunctionString(); }

Mantid::API::IFunction_sptr FunctionBrowser::getFunction() { return m_presenter->getFunction(); }

bool FunctionBrowser::hasFunction() const { return m_presenter->hasFunction(); }

/// Get the number of datasets
int FunctionBrowser::getNumberOfDatasets() const { return m_presenter->getNumberOfDatasets(); }

/// Get the names of datasets
QStringList FunctionBrowser::getDatasetNames() const { return m_presenter->getDatasetNames(); }

/// Get the names of the dataset domains
QStringList FunctionBrowser::getDatasetDomainNames() const { return m_presenter->getDatasetDomainNames(); }

/// Set new number of the datasets
/// @param n :: New value for the number of datasets.
void FunctionBrowser::setNumberOfDatasets(int n) { m_presenter->setNumberOfDatasets(n); }

/// Sets the datasets being fitted. They will be displayed by the
/// local parameter editing dialog.
/// @param datasetNames :: Names of the datasets
void FunctionBrowser::setDatasets(const QStringList &datasetNames) { m_presenter->setDatasets(datasetNames); }

/// Sets the datasets being fitted. They will be displayed by the
/// local parameter editing dialog.
/// @param datasets :: Names of workspaces to be fitted maped to a spectra list.
void FunctionBrowser::setDatasets(const QList<FunctionModelDataset> &datasets) { m_presenter->setDatasets(datasets); }

/**
 * Get value of a local parameter
 * @param parName :: Name of a parameter.
 * @param i :: Data set index.
 */
double FunctionBrowser::getLocalParameterValue(const QString &parName, int i) const {
  return m_presenter->getLocalParameterValue(parName, i);
}

void FunctionBrowser::setLocalParameterValue(const QString &parName, int i, double value) {
  m_presenter->setLocalParameterValue(parName, i, value);
}

void FunctionBrowser::setLocalParameterValue(const QString &parName, int i, double value, double error) {
  m_presenter->setLocalParameterValue(parName, i, value, error);
}

/// Get error of a local parameter
double FunctionBrowser::getLocalParameterError(const QString &parName, int i) const {
  return m_presenter->getLocalParameterValue(parName, i);
}

void FunctionBrowser::resetLocalParameters() {}

/// Set current dataset.
void FunctionBrowser::setCurrentDataset(int i) { m_presenter->setCurrentDataset(i); }

/// Remove local parameter values for a number of datasets.
/// @param indices :: A list of indices of datasets to remove.
void FunctionBrowser::removeDatasets(const QList<int> &indices) { m_presenter->removeDatasets(indices); }

/// Add some datasets to those already set.
/// @param names :: A list of names for the new datasets.
void FunctionBrowser::addDatasets(const QStringList &names) { m_presenter->addDatasets(names); }

/// Return the multidomain function for multi-dataset fitting
IFunction_sptr FunctionBrowser::getGlobalFunction() { return m_presenter->getFitFunction(); }

/// Fix/unfix a local parameter
/// @param parName :: Parameter name
/// @param i :: Index of a dataset.
/// @param fixed :: Make it fixed (true) or free (false)
void FunctionBrowser::setLocalParameterFixed(const QString &parName, int i, bool fixed) {
  m_presenter->setLocalParameterFixed(parName, i, fixed);
}

/// Check if a local parameter is fixed
/// @param parName :: Parameter name
/// @param i :: Index of a dataset.
bool FunctionBrowser::isLocalParameterFixed(const QString &parName, int i) const {
  return m_presenter->isLocalParameterFixed(parName, i);
}

/// Get the tie for a local parameter.
/// @param parName :: Parameter name
/// @param i :: Index of a dataset.
QString FunctionBrowser::getLocalParameterTie(const QString &parName, int i) const {
  return m_presenter->getLocalParameterTie(parName, i);
}

/// Set a tie for a local parameter.
/// @param parName :: Parameter name
/// @param i :: Index of a dataset.
/// @param tie :: A tie string.
void FunctionBrowser::setLocalParameterTie(const QString &parName, int i, QString tie) {
  m_presenter->setLocalParameterTie(parName, i, tie);
}

/// Update the interface to have the same parameter values as in a function.
/// @param fun :: A function to get parameter values from.
void FunctionBrowser::updateMultiDatasetParameters(const IFunction &fun) {
  m_presenter->updateMultiDatasetParameters(fun);
}

/// Update the interface to have the same attribute values as in a function.
/// @param fun :: A function to get attribute values from.
void FunctionBrowser::updateMultiDatasetAttributes(const IFunction &fun) {
  m_presenter->updateMultiDatasetAttributes(fun);
}

void FunctionBrowser::updateMultiDatasetParameters(const ITableWorkspace &paramTable) {
  auto const nRows = paramTable.rowCount();
  if (nRows == 0)
    return;

  auto const globalParameterNames = getGlobalParameters();
  for (auto &&name : globalParameterNames) {
    auto valueColumn = paramTable.getColumn(name.toStdString());
    auto errorColumn = paramTable.getColumn((name + "_Err").toStdString());
    setParameter(name, valueColumn->toDouble(0));
    setParameterError(name, errorColumn->toDouble(0));
  }

  auto const localParameterNames = getLocalParameters();
  for (auto &&name : localParameterNames) {
    auto valueColumn = paramTable.getColumn(name.toStdString());
    auto errorColumn = paramTable.getColumn((name + "_Err").toStdString());
    if (nRows > 1) {
      for (size_t i = 0; i < nRows; ++i) {
        setLocalParameterValue(name, static_cast<int>(i), valueColumn->toDouble(i), errorColumn->toDouble(i));
      }
    } else {
      auto const i = getCurrentDataset();
      setLocalParameterValue(name, i, valueColumn->toDouble(0), errorColumn->toDouble(0));
    }
  }
}

/// Get the index of the current dataset.
int FunctionBrowser::getCurrentDataset() const { return m_presenter->getCurrentDataset(); }

/// Resize the browser's columns
/// @param s0 :: New size for the first column (Parameter).
/// @param s1 :: New size for the second column (Value).
/// @param s2 :: New size for the third optional column (Global).
void FunctionBrowser::setColumnSizes(int s0, int s1, int s2) { m_presenter->setColumnSizes(s0, s1, s2); }

/**
 * Set the last column to stretch. This avoids a vertical scroll bar.
 * @param stretch :: A bool whether or not to stretch the last column.
 */
void FunctionBrowser::setStretchLastColumn(bool stretch) { m_presenter->setStretchLastColumn(stretch); }

/**
 * Set display of parameter errors on/off
 * @param enabled :: [input] On/off display of errors
 */
void FunctionBrowser::setErrorsEnabled(bool enabled) { m_presenter->setErrorsEnabled(enabled); }

/**
 * Clear all errors, if they are set
 */
void FunctionBrowser::clearErrors() { m_presenter->clearErrors(); }

QStringList FunctionBrowser::getGlobalParameters() const { return m_presenter->getGlobalParameters(); }

QStringList FunctionBrowser::getLocalParameters() const { return m_presenter->getLocalParameters(); }

void FunctionBrowser::setGlobalParameters(const QStringList &globals) { m_presenter->setGlobalParameters(globals); }

boost::optional<QString> FunctionBrowser::currentFunctionIndex() { return m_presenter->currentFunctionIndex(); }

FunctionTreeView *FunctionBrowser::view() const { return dynamic_cast<FunctionTreeView *>(m_presenter->view()); }

QString FunctionBrowser::getFitFunctionString() const { return m_presenter->getFitFunctionString(); }

void FunctionBrowser::setBackgroundA0(double value) { m_presenter->setBackgroundA0(value); }

void FunctionBrowser::hideGlobalCheckbox() { m_presenter->hideGlobals(); }

void FunctionBrowser::showGlobalCheckbox() { m_presenter->showGlobals(); }

} // namespace MantidQt::MantidWidgets
