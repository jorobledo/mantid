// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectDataAnalysisElwinTab.h"
#include "MantidAPI/FunctionFactory.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces::IDA;
using namespace testing;

class IndirectDataAnalysisElwinTabTest : public CxxTest::TestSuite {
public:
  /// Needed to make sure everything is initialized
  IndirectDataAnalysisElwinTabTest() = default;

  static IndirectDataAnalysisElwinTabTest *createSuite() { return new IndirectDataAnalysisElwinTabTest(); }

  static void destroySuite(IndirectDataAnalysisElwinTabTest *suite) { delete suite; }

  void setUp() override {
    m_elwinTab = std::make_unique<IndirectDataAnalysisElwinTab>();
    m_ads = std::make_unique<SetUpADSWithWorkspace>("WorkspaceName", createWorkspace(5));
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();
    m_elwinTab.reset();
  }

  void test_test_that_will_pass() { ASSERT_TRUE(true); }

  std::unique_ptr<IndirectDataAnalysisElwinTab> m_elwinTab;

  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
};