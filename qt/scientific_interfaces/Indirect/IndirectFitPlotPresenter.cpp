// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitPlotPresenter.h"
#include "IndirectSettingsHelper.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"

#include <QTimer>
#include <utility>

using namespace MantidQt::Widgets::MplCpp;

namespace MantidQt::CustomInterfaces::IDA {

struct HoldRedrawing {
  IIndirectFitPlotView *m_view;

  HoldRedrawing(IIndirectFitPlotView *view) : m_view(view) { m_view->allowRedraws(false); }
  ~HoldRedrawing() {
    m_view->allowRedraws(true);
    m_view->redrawPlots();
  }
};

using namespace Mantid::API;

IndirectFitPlotPresenter::IndirectFitPlotPresenter(IIndirectFitPlotView *view)
    : m_model(new IndirectFitPlotModel()), m_view(view), m_plotGuessInSeparateWindow(false),
      m_plotter(std::make_unique<ExternalPlotter>()) {
  connect(m_view, SIGNAL(selectedFitDataChanged(WorkspaceID)), this, SLOT(handleSelectedFitDataChanged(WorkspaceID)));

  connect(m_view, SIGNAL(plotSpectrumChanged(WorkspaceIndex)), this, SLOT(handlePlotSpectrumChanged(WorkspaceIndex)));

  connect(m_view, SIGNAL(plotCurrentPreview()), this, SLOT(plotCurrentPreview()));

  connect(m_view, SIGNAL(fitSelectedSpectrum()), this, SLOT(emitFitSingleSpectrum()));

  connect(m_view, SIGNAL(plotGuessChanged(bool)), this, SLOT(plotGuess(bool)));

  connect(m_view, SIGNAL(startXChanged(double)), this, SIGNAL(startXChanged(double)));
  connect(m_view, SIGNAL(endXChanged(double)), this, SIGNAL(endXChanged(double)));

  connect(m_view, SIGNAL(hwhmMaximumChanged(double)), this, SLOT(setHWHMMinimum(double)));
  connect(m_view, SIGNAL(hwhmMinimumChanged(double)), this, SLOT(setHWHMMaximum(double)));

  connect(m_view, SIGNAL(hwhmChanged(double, double)), this, SLOT(emitFWHMChanged(double, double)));
  connect(m_view, SIGNAL(backgroundChanged(double)), this, SIGNAL(backgroundChanged(double)));

  // updatePlots();
  // updateRangeSelectors();
  // updateAvailableSpectra();
}

void IndirectFitPlotPresenter::handleSelectedFitDataChanged(WorkspaceID workspaceID) {
  setActiveIndex(workspaceID);
  updateAvailableSpectra();
  updatePlots();
  updateGuess();
  emit selectedFitDataChanged(workspaceID);
}

void IndirectFitPlotPresenter::handlePlotSpectrumChanged(WorkspaceIndex spectrum) {
  setActiveSpectrum(spectrum);
  updatePlots();
  emit plotSpectrumChanged();
}

void IndirectFitPlotPresenter::watchADS(bool watch) { m_view->watchADS(watch); }

WorkspaceID IndirectFitPlotPresenter::getActiveWorkspaceID() const { return m_model->getActiveWorkspaceID(); }

WorkspaceIndex IndirectFitPlotPresenter::getActiveWorkspaceIndex() const { return m_model->getActiveWorkspaceIndex(); }

FitDomainIndex IndirectFitPlotPresenter::getSelectedDomainIndex() const { return m_model->getActiveDomainIndex(); }

bool IndirectFitPlotPresenter::isCurrentlySelected(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  return getActiveWorkspaceID() == workspaceID && getActiveWorkspaceIndex() == spectrum;
}

void IndirectFitPlotPresenter::setActiveIndex(WorkspaceID workspaceID) { m_model->setActiveIndex(workspaceID); }

void IndirectFitPlotPresenter::setActiveSpectrum(WorkspaceIndex spectrum) {
  m_model->setActiveSpectrum(spectrum);
  m_view->setPlotSpectrum(spectrum);
}

void IndirectFitPlotPresenter::disableSpectrumPlotSelection() { m_view->disableSpectrumPlotSelection(); }

void IndirectFitPlotPresenter::setStartX(double startX) { m_view->setFitRangeMinimum(startX); }

void IndirectFitPlotPresenter::setEndX(double endX) { m_view->setFitRangeMaximum(endX); }

void IndirectFitPlotPresenter::setXBounds(std::pair<double, double> const &bounds) {
  m_view->setFitRangeBounds(bounds);
}

void IndirectFitPlotPresenter::setFittingData(std::vector<IndirectFitData> *fittingData) {
  m_model->setFittingData(fittingData);
}

void IndirectFitPlotPresenter::setFitOutput(IIndirectFitOutput *fitOutput) { m_model->setFitOutput(fitOutput); }

void IndirectFitPlotPresenter::updatePlotSpectrum(WorkspaceIndex spectrum) {
  setActiveSpectrum(spectrum);
  updatePlots();
}

void IndirectFitPlotPresenter::updateRangeSelectors() {
  updateBackgroundSelector();
  updateHWHMSelector();
}

void IndirectFitPlotPresenter::setHWHMMaximum(double minimum) {
  m_view->setHWHMMaximum(m_model->calculateHWHMMaximum(minimum));
}

void IndirectFitPlotPresenter::setHWHMMinimum(double maximum) {
  m_view->setHWHMMinimum(m_model->calculateHWHMMinimum(maximum));
}

void IndirectFitPlotPresenter::enablePlotGuessInSeparateWindow() {
  m_plotGuessInSeparateWindow = true;
  const auto inputAndGuess = m_model->appendGuessToInput(m_model->getGuessWorkspace());
  m_plotter->plotSpectra(inputAndGuess->getName(), "0-1", IndirectSettingsHelper::externalPlotErrorBars());
}

void IndirectFitPlotPresenter::disablePlotGuessInSeparateWindow() {
  m_plotGuessInSeparateWindow = false;
  m_model->deleteExternalGuessWorkspace();
}

void IndirectFitPlotPresenter::appendLastDataToSelection(std::vector<std::string> displayNames) {
  const auto workspaceCount = displayNames.size();
  if (m_view->dataSelectionSize() == workspaceCount) {
    // if adding a spectra to an existing workspace, update all the combo box
    // entries.
    for (size_t i = 0; i < workspaceCount; i++) {
      m_view->setNameInDataSelection(displayNames[i], WorkspaceID(i));
    }
  } else
    m_view->appendToDataSelection(displayNames.back());
}

void IndirectFitPlotPresenter::updateDataSelection(std::vector<std::string> displayNames) {
  MantidQt::API::SignalBlocker blocker(m_view);
  m_view->clearDataSelection();
  const auto workspaceCount = displayNames.size();
  for (size_t i = 0; i < workspaceCount; ++i) {
    m_view->appendToDataSelection(displayNames[i]);
  }
  setActiveIndex(WorkspaceID{0});
  setActiveSpectrum(WorkspaceIndex{0});
  updateAvailableSpectra();
  emitSelectedFitDataChanged();
}

void IndirectFitPlotPresenter::updateAvailableSpectra() {
  if (m_model->getWorkspace()) {
    enableAllDataSelection();
    auto spectra = m_model->getSpectra(m_model->getActiveWorkspaceID());
    if (spectra.isContinuous()) {
      auto const minmax = spectra.getMinMax();
      m_view->setAvailableSpectra(minmax.first, minmax.second);
    } else {
      m_view->setAvailableSpectra(spectra.begin(), spectra.end());
    }
    setActiveSpectrum(m_view->getSelectedSpectrum());
  } else
    disableAllDataSelection();
}

void IndirectFitPlotPresenter::disableAllDataSelection() {
  m_view->enableSpectrumSelection(false);
  m_view->enableFitRangeSelection(false);
}

void IndirectFitPlotPresenter::enableAllDataSelection() {
  m_view->enableSpectrumSelection(true);
  m_view->enableFitRangeSelection(true);
}

void IndirectFitPlotPresenter::setFitFunction(Mantid::API::MultiDomainFunction_sptr function) {
  m_model->setFitFunction(function);
}

void IndirectFitPlotPresenter::setFitSingleSpectrumIsFitting(bool fitting) {
  m_view->setFitSingleSpectrumText(fitting ? "Fitting..." : "Fit Single Spectrum");
}

void IndirectFitPlotPresenter::setFitSingleSpectrumEnabled(bool enable) { m_view->setFitSingleSpectrumEnabled(enable); }

void IndirectFitPlotPresenter::updatePlots() {
  HoldRedrawing holdRedrawing(m_view);
  m_view->clearPreviews();
  plotLines();

  updateRangeSelectors();
  updateFitRangeSelector();
}

void IndirectFitPlotPresenter::updateFit() {
  HoldRedrawing holdRedrawing(m_view);
  updateGuess();
}

void IndirectFitPlotPresenter::plotLines() {
  if (auto const resultWorkspace = m_model->getResultWorkspace()) {
    plotInput(m_model->getWorkspace(), m_model->getActiveWorkspaceIndex());
    plotFit(resultWorkspace);
    updatePlotRange(m_model->getResultRange());
  } else if (auto const inputWorkspace = m_model->getWorkspace()) {
    plotInput(inputWorkspace);
    updatePlotRange(m_model->getWorkspaceRange());
  }
}

void IndirectFitPlotPresenter::plotInput(MatrixWorkspace_sptr workspace) {
  plotInput(std::move(workspace), m_model->getActiveWorkspaceIndex());
  if (auto doGuess = m_view->isPlotGuessChecked())
    plotGuess(doGuess);
}

void IndirectFitPlotPresenter::plotInput(MatrixWorkspace_sptr workspace, WorkspaceIndex spectrum) {
  m_view->plotInTopPreview("Sample", std::move(workspace), spectrum, Qt::black);
}

void IndirectFitPlotPresenter::plotFit(const MatrixWorkspace_sptr &workspace) {
  if (auto doGuess = m_view->isPlotGuessChecked())
    plotGuess(doGuess);
  plotFit(workspace, WorkspaceIndex{1});
  plotDifference(workspace, WorkspaceIndex{2});
}

void IndirectFitPlotPresenter::plotFit(MatrixWorkspace_sptr workspace, WorkspaceIndex spectrum) {
  m_view->plotInTopPreview("Fit", std::move(workspace), spectrum, Qt::red);
}

void IndirectFitPlotPresenter::plotDifference(MatrixWorkspace_sptr workspace, WorkspaceIndex spectrum) {
  m_view->plotInBottomPreview("Difference", std::move(workspace), spectrum, Qt::blue);
}

void IndirectFitPlotPresenter::updatePlotRange(const std::pair<double, double> &range) {
  MantidQt::API::SignalBlocker blocker(m_view);
  m_view->setFitRange(range.first, range.second);
  m_view->setHWHMRange(range.first, range.second);
}

void IndirectFitPlotPresenter::updateFitRangeSelector() {
  const auto range = m_model->getRange();
  m_view->setFitRangeMinimum(range.first);
  m_view->setFitRangeMaximum(range.second);
}

void IndirectFitPlotPresenter::plotCurrentPreview() {
  const auto inputWorkspace = m_model->getWorkspace();
  if (inputWorkspace && !inputWorkspace->getName().empty()) {
    plotSpectrum(m_model->getActiveWorkspaceIndex());
  } else
    m_view->displayMessage("Workspace not found - data may not be loaded.");
}

void IndirectFitPlotPresenter::updateGuess() {
  if (m_model->canCalculateGuess()) {
    m_view->enablePlotGuess(true);
    plotGuess(m_view->isPlotGuessChecked());
  } else {
    m_view->enablePlotGuess(false);
    clearGuess();
  }
}

void IndirectFitPlotPresenter::updateGuessAvailability() {
  if (m_model->canCalculateGuess())
    m_view->enablePlotGuess(true);
  else
    m_view->enablePlotGuess(false);
}

void IndirectFitPlotPresenter::plotGuess(bool doPlotGuess) {
  if (doPlotGuess) {
    const auto guessWorkspace = m_model->getGuessWorkspace();
    if (guessWorkspace->x(0).size() >= 2) {
      plotGuess(guessWorkspace);
      if (m_plotGuessInSeparateWindow)
        plotGuessInSeparateWindow(guessWorkspace);
    }
  } else if (m_plotGuessInSeparateWindow)
    plotGuessInSeparateWindow(m_model->getGuessWorkspace());
  else
    clearGuess();
}

void IndirectFitPlotPresenter::plotGuess(Mantid::API::MatrixWorkspace_sptr workspace) {
  m_view->plotInTopPreview("Guess", std::move(workspace), WorkspaceIndex{0}, Qt::green);
}

void IndirectFitPlotPresenter::plotGuessInSeparateWindow(const Mantid::API::MatrixWorkspace_sptr &workspace) {
  m_plotExternalGuessRunner.addCallback([this, workspace]() { m_model->appendGuessToInput(workspace); });
}

void IndirectFitPlotPresenter::clearGuess() {
  m_view->removeFromTopPreview("Guess");
  m_view->redrawPlots();
}

void IndirectFitPlotPresenter::updateHWHMSelector() {
  const auto hwhm = m_model->getFirstHWHM();
  m_view->setHWHMRangeVisible(hwhm ? true : false);

  if (hwhm)
    setHWHM(*hwhm);
}

void IndirectFitPlotPresenter::setHWHM(double hwhm) {
  const auto centre = m_model->getFirstPeakCentre().get_value_or(0.);
  m_view->setHWHMMaximum(centre + hwhm);
  m_view->setHWHMMinimum(centre - hwhm);
}

void IndirectFitPlotPresenter::updateBackgroundSelector() {
  const auto background = m_model->getFirstBackgroundLevel();
  m_view->setBackgroundRangeVisible(background ? true : false);

  if (background)
    m_view->setBackgroundLevel(*background);
}

void IndirectFitPlotPresenter::plotSpectrum(WorkspaceIndex spectrum) const {
  const auto resultWs = m_model->getResultWorkspace();
  const auto errorBars = IndirectSettingsHelper::externalPlotErrorBars();
  if (resultWs)
    m_plotter->plotSpectra(resultWs->getName(), "0-2", errorBars);
  else
    m_plotter->plotSpectra(m_model->getWorkspace()->getName(), std::to_string(spectrum.value), errorBars);
}

void IndirectFitPlotPresenter::emitFitSingleSpectrum() {
  emit fitSingleSpectrum(m_model->getActiveWorkspaceID(), m_model->getActiveWorkspaceIndex());
}

void IndirectFitPlotPresenter::emitFWHMChanged(double minimum, double maximum) { emit fwhmChanged(maximum - minimum); }

void IndirectFitPlotPresenter::emitSelectedFitDataChanged() {
  const auto index = m_view->getSelectedDataIndex();
  emit selectedFitDataChanged(index);
}

} // namespace MantidQt::CustomInterfaces::IDA
