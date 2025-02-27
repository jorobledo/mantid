// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtBatchView.h"
#include "GUI/Event/QtEventView.h"
#include "GUI/Preview/QtPreviewView.h"
#include "GUI/Runs/QtRunsView.h"
#include "GUI/Save/QtSaveView.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"

#include <QMessageBox>
#include <QMetaType>
#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

using API::BatchAlgorithmRunner;
using Mantid::API::IAlgorithm_sptr;

QtBatchView::QtBatchView(QWidget *parent) : QWidget(parent), m_batchAlgoRunner(this) {
  qRegisterMetaType<API::IConfiguredAlgorithm_sptr>("MantidQt::API::IConfiguredAlgorithm_sptr");
  initLayout();
  m_batchAlgoRunner.stopOnFailure(false);
  connectBatchAlgoRunnerSlots();
}

void QtBatchView::subscribe(JobRunnerSubscriber *notifyee) { m_notifyees.push_back(notifyee); }

void QtBatchView::initLayout() {
  m_ui.setupUi(this);

  m_runs = createRunsTab();
  m_ui.batchTabs->addTab(m_runs.get(), "Runs");

  m_eventHandling = createEventTab();
  m_ui.batchTabs->addTab(m_eventHandling.get(), "Event Handling");

  m_experiment = std::make_unique<QtExperimentView>(createReductionAlg(), this);
  m_ui.batchTabs->addTab(m_experiment.get(), "Experiment Settings");

  m_instrument = std::make_unique<QtInstrumentView>(createReductionAlg(), this);
  m_ui.batchTabs->addTab(m_instrument.get(), "Instrument Settings");

  m_preview = std::make_unique<QtPreviewView>(this);
  m_ui.batchTabs->addTab(m_preview.get(), "Reduction Preview");
#ifdef NDEBUG
  // Preview disabled in Release mode to prevent users "finding" it in the nightly
  m_ui.batchTabs->removeTab(m_ui.batchTabs->indexOf(m_preview.get()));
#endif // NDEBUG

  m_save = createSaveTab();
  m_ui.batchTabs->addTab(m_save.get(), "Save ASCII");
}

IExperimentView *QtBatchView::experiment() const { return m_experiment.get(); }

IInstrumentView *QtBatchView::instrument() const { return m_instrument.get(); }

IPreviewView *QtBatchView::preview() const { return m_preview.get(); }

IRunsView *QtBatchView::runs() const { return m_runs.get(); }

IEventView *QtBatchView::eventHandling() const { return m_eventHandling.get(); }

ISaveView *QtBatchView::save() const { return m_save.get(); }

void QtBatchView::clearAlgorithmQueue() { m_batchAlgoRunner.clearQueue(); }

void QtBatchView::setAlgorithmQueue(std::deque<API::IConfiguredAlgorithm_sptr> algorithms) {
  m_batchAlgoRunner.setQueue(algorithms);
}

void QtBatchView::connectBatchAlgoRunnerSlots() {
  connect(&m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(onBatchComplete(bool)));
  connect(&m_batchAlgoRunner, SIGNAL(batchCancelled()), this, SLOT(onBatchCancelled()));
  connect(&m_batchAlgoRunner, SIGNAL(algorithmStarted(MantidQt::API::IConfiguredAlgorithm_sptr)), this,
          SLOT(onAlgorithmStarted(MantidQt::API::IConfiguredAlgorithm_sptr)));
  connect(&m_batchAlgoRunner, SIGNAL(algorithmComplete(MantidQt::API::IConfiguredAlgorithm_sptr)), this,
          SLOT(onAlgorithmComplete(MantidQt::API::IConfiguredAlgorithm_sptr)));
  connect(&m_batchAlgoRunner, SIGNAL(algorithmError(MantidQt::API::IConfiguredAlgorithm_sptr, std::string)), this,
          SLOT(onAlgorithmError(MantidQt::API::IConfiguredAlgorithm_sptr, std::string)));
}

void QtBatchView::executeAlgorithmQueue() { m_batchAlgoRunner.executeBatchAsync(); }

void QtBatchView::cancelAlgorithmQueue() { m_batchAlgoRunner.cancelBatch(); }

void QtBatchView::onBatchComplete(bool error) {
  for (auto notifyee : m_notifyees) {
    notifyee->notifyBatchComplete(error);
  }
}

void QtBatchView::onBatchCancelled() {
  for (auto notifyee : m_notifyees) {
    notifyee->notifyBatchCancelled();
  }
}

void QtBatchView::onAlgorithmStarted(API::IConfiguredAlgorithm_sptr algorithm) {
  for (auto notifyee : m_notifyees) {
    notifyee->notifyAlgorithmStarted(algorithm);
  }
}

void QtBatchView::onAlgorithmComplete(API::IConfiguredAlgorithm_sptr algorithm) {
  for (auto notifyee : m_notifyees) {
    notifyee->notifyAlgorithmComplete(algorithm);
  }
}

void QtBatchView::onAlgorithmError(API::IConfiguredAlgorithm_sptr algorithm, const std::string &message) {
  for (auto notifyee : m_notifyees) {
    notifyee->notifyAlgorithmError(algorithm, message);
  }
}

std::unique_ptr<QtRunsView> QtBatchView::createRunsTab() {
  auto instruments = std::vector<std::string>({{"INTER", "SURF", "CRISP", "POLREF", "OFFSPEC"}});
  return std::make_unique<QtRunsView>(this, RunsTableViewFactory(instruments));
}

std::unique_ptr<QtEventView> QtBatchView::createEventTab() { return std::make_unique<QtEventView>(this); }

IAlgorithm_sptr QtBatchView::createReductionAlg() {
  return Mantid::API::AlgorithmManager::Instance().create("ReflectometryISISLoadAndProcess");
}

std::unique_ptr<QtSaveView> QtBatchView::createSaveTab() { return std::make_unique<QtSaveView>(this); }
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
