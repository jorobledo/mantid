// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Batch/BatchPresenter.h"
#include "../../../ISISReflectometry/TestHelpers/ModelCreationHelper.h"
#include "../MainWindow/MockMainWindowPresenter.h"
#include "../ReflMockObjects.h"
#include "MantidAPI/FrameworkManager.h"
#include "MockBatchView.h"
#include "MockPreviewPresenter.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace MantidQt::CustomInterfaces::ISISReflectometry::ModelCreationHelper;
using MantidQt::API::IConfiguredAlgorithm_sptr;
using testing::_;
using testing::AtLeast;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::ReturnRef;

class BatchPresenterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BatchPresenterTest *createSuite() { return new BatchPresenterTest(); }
  static void destroySuite(BatchPresenterTest *suite) { delete suite; }

  BatchPresenterTest()
      : m_view(), m_jobManager(nullptr), m_instruments{"INTER", "OFFSPEC", "POLREF", "SURF", "CRISP"}, m_tolerance(0.1),
        m_experiment(makeEmptyExperiment()), m_instrument(makeEmptyInstrument()),
        m_runsTable(m_instruments, 0.1, ReductionJobs()),
        m_slicing(), m_mockAlgorithmsList{std::make_shared<MockBatchJobAlgorithm>()} {
    Mantid::API::FrameworkManager::Instance();
  }

  void testPresenterSubscribesToJobRunner() {
    EXPECT_CALL(m_jobRunner, subscribe(_)).Times(1);
    auto presenter = makePresenter();
    verifyAndClear();
  }

  void testInitInstrumentListUpdatesRunsPresenter() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsPresenter, initInstrumentList()).Times(1);
    presenter->initInstrumentList();
    verifyAndClear();
  }

  void testMainPresenterUpdatedWhenChangeInstrumentRequested() {
    auto presenter = makePresenter();
    auto const instrument = std::string("POLREF");
    EXPECT_CALL(m_mainPresenter, notifyChangeInstrumentRequested(instrument)).Times(1);
    presenter->notifyChangeInstrumentRequested(instrument);
    verifyAndClear();
  }

  void testChildPresentersAreUpdatedWhenInstrumentChanged() {
    auto presenter = makePresenter();
    auto const instrument = std::string("POLREF");
    EXPECT_CALL(*m_runsPresenter, notifyInstrumentChanged(instrument)).Times(1);
    EXPECT_CALL(*m_experimentPresenter, notifyInstrumentChanged(instrument)).Times(1);
    EXPECT_CALL(*m_instrumentPresenter, notifyInstrumentChanged(instrument)).Times(1);
    presenter->notifyInstrumentChanged(instrument);
    verifyAndClear();
  }

  void testMainPresenterUpdatedWhenUpdateInstrumentRequested() {
    auto presenter = makePresenter();
    auto const instrument = std::string("POLREF");
    EXPECT_CALL(m_mainPresenter, notifyUpdateInstrumentRequested()).Times(1);
    presenter->notifyUpdateInstrumentRequested();
    verifyAndClear();
  }

  void testChildPresentersUpdatedWhenSettingsChanged() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsPresenter, settingsChanged()).Times(1);
    presenter->notifySettingsChanged();
    verifyAndClear();
  }

  void testModelUpdatedWhenReductionResumed() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_jobManager, notifyReductionResumed()).Times(1);
    presenter->notifyResumeReductionRequested();
    verifyAndClear();
  }

  void testBatchIsExecutedWhenReductionResumed() {
    auto presenter = makePresenter();
    expectBatchIsExecuted();
    presenter->notifyResumeReductionRequested();
    verifyAndClear();
  }

  void testOtherPresentersUpdatedWhenReductionResumed() {
    auto presenter = makePresenter();
    expectReductionResumed();
    presenter->notifyResumeReductionRequested();
    verifyAndClear();
  }

  void testJobManagerGetProcessAll() {
    auto presenter = makePresenter();
    TS_ASSERT_EQUALS(m_jobManager->getProcessAll(), false);
    expectReductionResumed();
    presenter->notifyResumeReductionRequested();
    verifyAndClear();
  }

  void testJobManagerGetProcessPartial() {
    auto presenter = makePresenter();
    TS_ASSERT_EQUALS(m_jobManager->getProcessPartial(), false);
    expectReductionResumed();
    presenter->notifyResumeReductionRequested();
    verifyAndClear();
  }

  void testWarnProcessAllWhenReductionResumedOptionChecked() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_jobManager, getProcessAll()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_jobManager, notifyReductionResumed()).Times(1);
    EXPECT_CALL(m_mainPresenter, isProcessAllPrevented()).Times(1).WillOnce(Return(true));
    presenter->notifyResumeReductionRequested();
    verifyAndClear();
  }

  void testNoWarnProcessAllWhenReductionResumedOptionUnchecked() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_jobManager, getProcessAll()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_jobManager, notifyReductionResumed()).Times(1);
    EXPECT_CALL(m_mainPresenter, isProcessAllPrevented()).Times(1).WillOnce(Return(false));
    presenter->notifyResumeReductionRequested();
    verifyAndClear();
  }

  void testWarnProcessPartialGroupWhenReductionResumedOptionChecked() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_jobManager, getProcessPartial()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_jobManager, notifyReductionResumed()).Times(1);
    EXPECT_CALL(m_mainPresenter, isProcessPartialGroupPrevented()).Times(1).WillOnce(Return(true));
    presenter->notifyResumeReductionRequested();
    verifyAndClear();
  }

  void testNoWarnProcessPartialGroupWhenReductionResumedOptionUnchecked() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_jobManager, getProcessPartial()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_jobManager, notifyReductionResumed()).Times(1);
    EXPECT_CALL(m_mainPresenter, isProcessPartialGroupPrevented()).Times(1).WillOnce(Return(false));
    presenter->notifyResumeReductionRequested();
    verifyAndClear();
  }

  void testChildPresentersUpdatedWhenAnyBatchReductionResumed() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsPresenter, notifyAnyBatchReductionResumed()).Times(1);
    presenter->notifyAnyBatchReductionResumed();
    verifyAndClear();
  }

  void testChildPresentersUpdatedWhenAnyBatchReductionPaused() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsPresenter, notifyAnyBatchReductionPaused()).Times(1);
    presenter->notifyAnyBatchReductionPaused();
    verifyAndClear();
  }

  void testChildPresentersUpdatedWhenAnyBatchAutoreductionResumed() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsPresenter, notifyAnyBatchAutoreductionResumed()).Times(1);
    presenter->notifyAnyBatchAutoreductionResumed();
    verifyAndClear();
  }

  void testChildPresentersUpdatedWhenAnyBatchAutoreductionPaused() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsPresenter, notifyAnyBatchAutoreductionPaused()).Times(1);
    presenter->notifyAnyBatchAutoreductionPaused();
    verifyAndClear();
  }

  void testMainPresenterQueriedWhenCheckingAnyBatchProcessing() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_mainPresenter, isAnyBatchProcessing()).Times(1).WillOnce(Return(true));
    auto result = presenter->isAnyBatchProcessing();
    TS_ASSERT_EQUALS(result, true);
    verifyAndClear();
  }

  void testMainPresenterQueriedWhenCheckingAnyBatchAutoreducing() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_mainPresenter, isAnyBatchAutoreducing()).Times(1).WillOnce(Return(true));
    auto result = presenter->isAnyBatchAutoreducing();
    TS_ASSERT_EQUALS(result, true);
    verifyAndClear();
  }

  void testAutoreductionCompletedWhenReductionResumedWithNoRemainingJobs() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_jobManager, getAlgorithms()).Times(1).WillOnce(Return(std::deque<IConfiguredAlgorithm_sptr>()));
    EXPECT_CALL(*m_jobManager, isAutoreducing()).Times(AtLeast(1)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_runsPresenter, autoreductionCompleted()).Times(1);
    presenter->notifyResumeReductionRequested();
    verifyAndClear();
  }

  void testAutoreductionNotCompletedWhenReductionResumedWithRemainingJobs() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsPresenter, autoreductionCompleted()).Times(0);
    presenter->notifyResumeReductionRequested();
    verifyAndClear();
  }

  void testBatchIsCancelledWhenReductionPaused() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_jobRunner, cancelAlgorithmQueue()).Times(1);
    presenter->notifyPauseReductionRequested();
    verifyAndClear();
  }

  void testModelUpdatedWhenBatchCancelled() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_jobManager, notifyReductionPaused()).Times(1);
    presenter->notifyBatchCancelled();
    verifyAndClear();
  }

  void testChildPresentersUpdatedWhenBatchCancelled() {
    auto presenter = makePresenter();
    expectReductionPaused();
    expectAutoreductionPaused();
    presenter->notifyBatchCancelled();
    verifyAndClear();
  }

  void testModelUpdatedWhenAutoreductionResumed() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_jobManager, notifyAutoreductionResumed()).Times(1);
    EXPECT_CALL(*m_jobManager, notifyAutoreductionPaused()).Times(0);
    presenter->notifyResumeAutoreductionRequested();
    verifyAndClear();
  }

  void testRunsPresenterCalledWhenAutoreductionResumed() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsPresenter, resumeAutoreduction()).Times(1);
    presenter->notifyResumeAutoreductionRequested();
    verifyAndClear();
  }

  void testModelResetWhenAutoreductionCancelled() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsPresenter, resumeAutoreduction()).Times(1).WillOnce(Return(false));
    EXPECT_CALL(*m_jobManager, notifyAutoreductionPaused()).Times(1);
    presenter->notifyResumeAutoreductionRequested();
    verifyAndClear();
  }

  void testOtherPresentersUpdatedWhenAutoreductionResumed() {
    auto presenter = makePresenter();
    expectAutoreductionResumed();
    presenter->notifyResumeAutoreductionRequested();
    verifyAndClear();
  }

  void testChildPresentersNotUpdatedWhenAutoreductionCanelled() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsPresenter, resumeAutoreduction()).Times(1).WillOnce(Return(false));
    EXPECT_CALL(*m_savePresenter, notifyAutoreductionResumed()).Times(0);
    EXPECT_CALL(*m_eventPresenter, notifyAutoreductionResumed()).Times(0);
    EXPECT_CALL(*m_experimentPresenter, notifyAutoreductionResumed()).Times(0);
    EXPECT_CALL(*m_instrumentPresenter, notifyAutoreductionResumed()).Times(0);
    EXPECT_CALL(*m_runsPresenter, notifyAutoreductionResumed()).Times(0);
    presenter->notifyResumeAutoreductionRequested();
    verifyAndClear();
  }

  void testModelUpdatedWhenAutoreductionPaused() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_jobManager, notifyAutoreductionPaused()).Times(1);
    presenter->notifyPauseAutoreductionRequested();
    verifyAndClear();
  }

  void testBatchIsCancelledWhenAutoreductionPaused() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_jobRunner, cancelAlgorithmQueue()).Times(1);
    presenter->notifyPauseAutoreductionRequested();
    verifyAndClear();
  }

  void testOtherPresentersUpdatedWhenAutoreductionPaused() {
    auto presenter = makePresenter();
    expectAutoreductionPaused();
    presenter->notifyPauseAutoreductionRequested();
    verifyAndClear();
  }

  void testAutoreductionComplete() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsPresenter, autoreductionCompleted()).Times(1);
    EXPECT_CALL(*m_runsPresenter, notifyRowStateChanged()).Times(1);
    presenter->notifyAutoreductionCompleted();
    verifyAndClear();
  }

  void testNextBatchIsStartedWhenBatchFinished() {
    auto presenter = makePresenter();
    expectBatchIsExecuted();
    presenter->notifyBatchComplete(false);
    verifyAndClear();
  }

  void testChildPresentersUpdatedWhenBatchFinishedAndNothingLeftToProcess() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_jobManager, getAlgorithms()).Times(1).WillOnce(Return(std::deque<IConfiguredAlgorithm_sptr>()));
    expectReductionPaused();
    presenter->notifyBatchComplete(false);
    verifyAndClear();
  }

  void testNotifyAlgorithmStarted() {
    auto presenter = makePresenter();
    IConfiguredAlgorithm_sptr algorithm = std::make_shared<MockBatchJobAlgorithm>();
    auto row = makeRow();
    EXPECT_CALL(*m_jobManager, getRunsTableItem(algorithm)).Times(1).WillOnce(Return(row));
    EXPECT_CALL(*m_jobManager, algorithmStarted(algorithm)).Times(1);
    EXPECT_CALL(*m_runsPresenter, notifyRowOutputsChanged(_)).Times(1);
    EXPECT_CALL(*m_runsPresenter, notifyRowStateChanged(_)).Times(1);
    presenter->notifyAlgorithmStarted(algorithm);
    verifyAndClear();
  }

  void testNotifyAlgorithmComplete() {
    auto presenter = makePresenter();
    IConfiguredAlgorithm_sptr algorithm = std::make_shared<MockBatchJobAlgorithm>();
    auto row = makeRow();
    EXPECT_CALL(*m_jobManager, getRunsTableItem(algorithm)).Times(1).WillOnce(Return(row));
    EXPECT_CALL(*m_jobManager, algorithmComplete(algorithm)).Times(1);
    EXPECT_CALL(*m_runsPresenter, notifyRowOutputsChanged(_)).Times(1);
    EXPECT_CALL(*m_runsPresenter, notifyRowStateChanged(_)).Times(1);
    presenter->notifyAlgorithmComplete(algorithm);
    verifyAndClear();
  }

  void testNotifyAlgorithmStartedSkipsNonItems() {
    auto presenter = makePresenter();
    IConfiguredAlgorithm_sptr algorithm = std::make_shared<MockBatchJobAlgorithm>();
    EXPECT_CALL(*m_jobManager, getRunsTableItem(algorithm)).Times(1).WillOnce(Return(boost::none));
    EXPECT_CALL(*m_jobManager, algorithmStarted(_)).Times(0);
    EXPECT_CALL(*m_runsPresenter, notifyRowOutputsChanged(_)).Times(0);
    EXPECT_CALL(*m_runsPresenter, notifyRowStateChanged(_)).Times(0);
    presenter->notifyAlgorithmStarted(algorithm);
    verifyAndClear();
  }

  void testNotifyAlgorithmCompleteSkipsNonItems() {
    auto presenter = makePresenter();
    IConfiguredAlgorithm_sptr algorithm = std::make_shared<MockBatchJobAlgorithm>();
    EXPECT_CALL(*m_jobManager, getRunsTableItem(algorithm)).Times(1).WillOnce(Return(boost::none));
    EXPECT_CALL(*m_jobManager, algorithmComplete(_)).Times(0);
    EXPECT_CALL(*m_runsPresenter, notifyRowOutputsChanged(_)).Times(0);
    EXPECT_CALL(*m_runsPresenter, notifyRowStateChanged(_)).Times(0);
    presenter->notifyAlgorithmComplete(algorithm);
    verifyAndClear();
  }

  void testNotifyAlgorithmErrorSkipsNonItems() {
    auto presenter = makePresenter();
    IConfiguredAlgorithm_sptr algorithm = std::make_shared<MockBatchJobAlgorithm>();
    EXPECT_CALL(*m_jobManager, getRunsTableItem(algorithm)).Times(1).WillOnce(Return(boost::none));
    EXPECT_CALL(*m_jobManager, algorithmError(_, _)).Times(0);
    EXPECT_CALL(*m_runsPresenter, notifyRowOutputsChanged(_)).Times(0);
    EXPECT_CALL(*m_runsPresenter, notifyRowStateChanged(_)).Times(0);
    presenter->notifyAlgorithmError(algorithm, "");
    verifyAndClear();
  }

  void testOutputWorkspacesSavedOnAlgorithmComplete() {
    auto presenter = makePresenter();
    IConfiguredAlgorithm_sptr algorithm = std::make_shared<MockBatchJobAlgorithm>();
    EXPECT_CALL(*m_savePresenter, shouldAutosave()).Times(1).WillOnce(Return(true));
    auto const workspaces = std::vector<std::string>{"test1", "test2"};
    auto row = makeRow();
    EXPECT_CALL(*m_jobManager, getRunsTableItem(algorithm)).Times(1).WillOnce(Return(row));
    EXPECT_CALL(*m_jobManager, algorithmComplete(algorithm)).Times(1);
    EXPECT_CALL(*m_jobManager, algorithmOutputWorkspacesToSave(algorithm)).Times(1).WillOnce(Return(workspaces));
    EXPECT_CALL(*m_savePresenter, saveWorkspaces(workspaces)).Times(1);
    presenter->notifyAlgorithmComplete(algorithm);
    verifyAndClear();
  }

  void testOutputWorkspacesNotSavedIfAutosaveDisabled() {
    auto presenter = makePresenter();
    IConfiguredAlgorithm_sptr algorithm = std::make_shared<MockBatchJobAlgorithm>();
    EXPECT_CALL(*m_savePresenter, shouldAutosave()).Times(1).WillOnce(Return(false));
    auto row = makeRow();
    EXPECT_CALL(*m_jobManager, getRunsTableItem(algorithm)).Times(1).WillOnce(Return(row));
    EXPECT_CALL(*m_jobManager, algorithmComplete(algorithm)).Times(1);
    EXPECT_CALL(*m_jobManager, algorithmOutputWorkspacesToSave(_)).Times(0);
    EXPECT_CALL(*m_savePresenter, saveWorkspaces(_)).Times(0);
    presenter->notifyAlgorithmComplete(algorithm);
    verifyAndClear();
  }

  void testNotifyAlgorithmError() {
    auto presenter = makePresenter();
    IConfiguredAlgorithm_sptr algorithm = std::make_shared<MockBatchJobAlgorithm>();
    auto const errorMessage = std::string("test error");
    auto row = makeRow();
    EXPECT_CALL(*m_jobManager, getRunsTableItem(algorithm)).Times(1).WillOnce(Return(row));
    EXPECT_CALL(*m_jobManager, algorithmError(algorithm, errorMessage)).Times(1);
    EXPECT_CALL(*m_runsPresenter, notifyRowStateChanged(_)).Times(1);
    EXPECT_CALL(*m_runsPresenter, notifyRowOutputsChanged(_)).Times(1);
    presenter->notifyAlgorithmError(algorithm, errorMessage);
    verifyAndClear();
  }

  void testModelUpdatedWhenWorkspaceDeleted() {
    auto presenter = makePresenter();
    auto name = std::string("test_workspace");
    EXPECT_CALL(*m_jobManager, notifyWorkspaceDeleted(name)).Times(1);
    presenter->postDeleteHandle(name);
    verifyAndClear();
  }

  void testRowStateUpdatedWhenWorkspaceDeleted() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsPresenter, notifyRowOutputsChanged(_)).Times(1);
    EXPECT_CALL(*m_runsPresenter, notifyRowStateChanged(_)).Times(1);
    presenter->postDeleteHandle("");
    verifyAndClear();
  }

  void testModelUpdatedWhenWorkspaceRenamed() {
    auto presenter = makePresenter();
    auto oldName = std::string("test_workspace1");
    auto newName = std::string("test_workspace2");
    EXPECT_CALL(*m_jobManager, notifyWorkspaceRenamed(oldName, newName)).Times(1);
    presenter->renameHandle(oldName, newName);
    verifyAndClear();
  }

  void testRowStateUpdatedWhenWorkspaceRenamed() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsPresenter, notifyRowOutputsChanged(_)).Times(1);
    EXPECT_CALL(*m_runsPresenter, notifyRowStateChanged(_)).Times(1);
    presenter->renameHandle("", "");
    verifyAndClear();
  }

  void testModelUpdatedWhenWorkspacesCleared() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_jobManager, notifyAllWorkspacesDeleted()).Times(1);
    presenter->clearADSHandle();
    verifyAndClear();
  }

  void testRowStateUpdatedWhenWorkspacesCleared() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsPresenter, notifyRowOutputsChanged()).Times(1);
    EXPECT_CALL(*m_runsPresenter, notifyRowStateChanged()).Times(1);
    presenter->clearADSHandle();
    verifyAndClear();
  }

  void testPercentCompleteIsRequestedFromJobManager() {
    auto presenter = makePresenter();
    auto progress = 33;
    EXPECT_CALL(*m_jobManager, percentComplete()).Times(1).WillOnce(Return(progress));
    TS_ASSERT_EQUALS(presenter->percentComplete(), progress);
    verifyAndClear();
  }

  void testRunsPresenterNotifiesSetRoundPrecision() {
    auto presenter = makePresenter();
    auto prec = 2;
    EXPECT_CALL(*m_runsPresenter, setRoundPrecision(prec));
    presenter->notifySetRoundPrecision(prec);
    verifyAndClear();
  }

  void testRunsPresenterNotifiesResetRoundPrecision() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsPresenter, resetRoundPrecision());
    presenter->notifyResetRoundPrecision();
    verifyAndClear();
  }

  void testNotifyBatchLoaded() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsPresenter, notifyBatchLoaded());
    presenter->notifyBatchLoaded();
    verifyAndClear();
  }

private:
  NiceMock<MockBatchView> m_view;
  NiceMock<MockBatchJobManager> *m_jobManager;
  NiceMock<MockJobRunner> m_jobRunner;
  NiceMock<MockMainWindowPresenter> m_mainPresenter;
  NiceMock<MockRunsPresenter> *m_runsPresenter;
  NiceMock<MockEventPresenter> *m_eventPresenter;
  NiceMock<MockExperimentPresenter> *m_experimentPresenter;
  NiceMock<MockInstrumentPresenter> *m_instrumentPresenter;
  NiceMock<MockSavePresenter> *m_savePresenter;
  NiceMock<MockPreviewPresenter> *m_previewPresenter;
  std::vector<std::string> m_instruments;
  double m_tolerance;
  Experiment m_experiment;
  Instrument m_instrument;
  RunsTable m_runsTable;
  Slicing m_slicing;
  std::deque<IConfiguredAlgorithm_sptr> m_mockAlgorithmsList;

  class BatchPresenterFriend : public BatchPresenter {
    friend class BatchPresenterTest;

  public:
    BatchPresenterFriend(IBatchView *view, std::unique_ptr<Batch> model, IJobRunner *jobRunner,
                         std::unique_ptr<IRunsPresenter> runsPresenter, std::unique_ptr<IEventPresenter> eventPresenter,
                         std::unique_ptr<IExperimentPresenter> experimentPresenter,
                         std::unique_ptr<IInstrumentPresenter> instrumentPresenter,
                         std::unique_ptr<ISavePresenter> savePresenter,
                         std::unique_ptr<IPreviewPresenter> previewPresenter)
        : BatchPresenter(view, std::move(model), jobRunner, std::move(runsPresenter), std::move(eventPresenter),
                         std::move(experimentPresenter), std::move(instrumentPresenter), std::move(savePresenter),
                         std::move(previewPresenter)) {}
  };

  RunsTable makeRunsTable() { return RunsTable(m_instruments, m_tolerance, ReductionJobs()); }

  std::unique_ptr<Batch> makeModel() {
    return std::make_unique<Batch>(m_experiment, m_instrument, m_runsTable, m_slicing);
  }

  std::unique_ptr<BatchPresenterFriend> makePresenter() {
    // Create pointers to the child presenters and pass them into the batch
    auto runsPresenter = std::make_unique<NiceMock<MockRunsPresenter>>();
    auto eventPresenter = std::make_unique<NiceMock<MockEventPresenter>>();
    auto experimentPresenter = std::make_unique<NiceMock<MockExperimentPresenter>>();
    auto instrumentPresenter = std::make_unique<NiceMock<MockInstrumentPresenter>>();
    auto savePresenter = std::make_unique<NiceMock<MockSavePresenter>>();
    auto previewPresenter = std::make_unique<NiceMock<MockPreviewPresenter>>();
    m_runsPresenter = runsPresenter.get();
    m_eventPresenter = eventPresenter.get();
    m_experimentPresenter = experimentPresenter.get();
    m_instrumentPresenter = instrumentPresenter.get();
    m_savePresenter = savePresenter.get();
    m_previewPresenter = previewPresenter.get();
    // Create the batch presenter
    auto presenter = std::make_unique<BatchPresenterFriend>(
        &m_view, makeModel(), &m_jobRunner, std::move(runsPresenter), std::move(eventPresenter),
        std::move(experimentPresenter), std::move(instrumentPresenter), std::move(savePresenter),
        std::move(previewPresenter));
    presenter->acceptMainPresenter(&m_mainPresenter);
    // Replace the constructed job runner with a mock
    m_jobManager = new NiceMock<MockBatchJobManager>();
    presenter->m_jobManager.reset(m_jobManager);
    // The mock job runner should by default return our default algorithms list
    ON_CALL(*m_jobManager, getAlgorithms()).WillByDefault(Return(m_mockAlgorithmsList));
    ON_CALL(*m_jobManager, getProcessAll()).WillByDefault(Return(false));
    ON_CALL(*m_jobManager, getProcessPartial()).WillByDefault(Return(false));
    // The mock runs presenter should by default return true when autoreduction
    // is resumed
    ON_CALL(*m_runsPresenter, resumeAutoreduction()).WillByDefault(Return(true));
    return presenter;
  }

  void verifyAndClear() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_runsPresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_eventPresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_experimentPresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_instrumentPresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_savePresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_previewPresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_jobManager));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_mainPresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_jobRunner));
  }

  void expectReductionResumed() {
    EXPECT_CALL(*m_savePresenter, notifyReductionResumed()).Times(1);
    EXPECT_CALL(*m_eventPresenter, notifyReductionResumed()).Times(1);
    EXPECT_CALL(*m_experimentPresenter, notifyReductionResumed()).Times(1);
    EXPECT_CALL(*m_instrumentPresenter, notifyReductionResumed()).Times(1);
    EXPECT_CALL(*m_runsPresenter, notifyReductionResumed()).Times(1);
    EXPECT_CALL(m_mainPresenter, notifyAnyBatchReductionResumed()).Times(1);
  }

  void expectReductionPaused() {
    EXPECT_CALL(*m_savePresenter, notifyReductionPaused()).Times(1);
    EXPECT_CALL(*m_eventPresenter, notifyReductionPaused()).Times(1);
    EXPECT_CALL(*m_experimentPresenter, notifyReductionPaused()).Times(1);
    EXPECT_CALL(*m_instrumentPresenter, notifyReductionPaused()).Times(1);
    EXPECT_CALL(*m_runsPresenter, notifyReductionPaused()).Times(1);
  }

  void expectAutoreductionResumed() {
    EXPECT_CALL(*m_savePresenter, notifyAutoreductionResumed()).Times(1);
    EXPECT_CALL(*m_eventPresenter, notifyAutoreductionResumed()).Times(1);
    EXPECT_CALL(*m_experimentPresenter, notifyAutoreductionResumed()).Times(1);
    EXPECT_CALL(*m_instrumentPresenter, notifyAutoreductionResumed()).Times(1);
    EXPECT_CALL(*m_runsPresenter, notifyAutoreductionResumed()).Times(1);
    EXPECT_CALL(*m_runsPresenter, notifyRowStateChanged()).Times(1);
    EXPECT_CALL(m_mainPresenter, notifyAnyBatchAutoreductionResumed()).Times(1);
  }

  void expectAutoreductionPaused() {
    EXPECT_CALL(*m_savePresenter, notifyAutoreductionPaused()).Times(1);
    EXPECT_CALL(*m_eventPresenter, notifyAutoreductionPaused()).Times(1);
    EXPECT_CALL(*m_experimentPresenter, notifyAutoreductionPaused()).Times(1);
    EXPECT_CALL(*m_instrumentPresenter, notifyAutoreductionPaused()).Times(1);
    EXPECT_CALL(*m_runsPresenter, notifyAutoreductionPaused()).Times(1);
    EXPECT_CALL(m_mainPresenter, notifyAnyBatchAutoreductionPaused()).Times(1);
  }

  void expectBatchIsExecuted() {
    EXPECT_CALL(*m_jobManager, getAlgorithms()).Times(1);
    EXPECT_CALL(m_jobRunner, clearAlgorithmQueue()).Times(1);
    EXPECT_CALL(m_jobRunner, setAlgorithmQueue(m_mockAlgorithmsList)).Times(1);
    EXPECT_CALL(m_jobRunner, executeAlgorithmQueue()).Times(1);
  }
};
