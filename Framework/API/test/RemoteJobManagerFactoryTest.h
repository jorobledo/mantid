// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/RemoteJobManagerFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"

using namespace Mantid::API;

// Just a minimal implementation of IRemoteJobManager, sufficient for the
// factory
// TODO: use gmock for this
class FakeJM : public IRemoteJobManager {
public:
  void authenticate(const std::string & /*username*/, const std::string & /*password*/) override {}

  void logout(const std::string & /*username*/) override {}

  std::string submitRemoteJob(const std::string & /*transactionID*/, const std::string & /*runnable*/,
                              const std::string & /*param*/, const std::string & /*taskName*/ = "",
                              const int /*numNodes*/ = 1, const int /*coresPerNode*/ = 1) override {
    return "";
  }

  void downloadRemoteFile(const std::string & /*transactionID*/, const std::string & /*remoteFileName*/,
                          const std::string & /*localFileName*/) override {}

  std::vector<RemoteJobInfo> queryAllRemoteJobs() const override { return std::vector<RemoteJobInfo>(); }

  std::vector<std::string> queryRemoteFile(const std::string & /*transactionID*/) const override {
    return std::vector<std::string>();
  }

  RemoteJobInfo queryRemoteJob(const std::string & /*jobID*/) const override { return RemoteJobInfo(); }

  std::string startRemoteTransaction() override { return ""; }

  void stopRemoteTransaction(const std::string & /*transactionID*/) override {}

  void abortRemoteJob(const std::string & /*jobID*/) override {}

  void uploadRemoteFile(const std::string & /*transactionID*/, const std::string & /*remoteFileName*/,
                        const std::string & /*localFileName*/) override {}
};

class FakeJMDeriv : public FakeJM {};

class FakeJM3 : public FakeJMDeriv {};

DECLARE_REMOTEJOBMANAGER(FakeJM)
DECLARE_REMOTEJOBMANAGER(FakeJMDeriv)
DECLARE_REMOTEJOBMANAGER(FakeJM3)

class RemoteJobManagerFactoryTest : public CxxTest::TestSuite {
public:
  void test_unsubscribeDeclared() {
    // subscribed with DECLARE_...
    TS_ASSERT_THROWS_NOTHING(Mantid::API::RemoteJobManagerFactory::Instance().unsubscribe("FakeJM"));
    TS_ASSERT_THROWS_NOTHING(Mantid::API::RemoteJobManagerFactory::Instance().unsubscribe("FakeJMDeriv"));
    TS_ASSERT_THROWS_NOTHING(Mantid::API::RemoteJobManagerFactory::Instance().unsubscribe("FakeJM3"));
  }

  void test_unsubscribed() {
    IRemoteJobManager_sptr jobManager;
    TSM_ASSERT_THROWS("create() with inexistent and unsubscribed class should "
                      "throw",
                      jobManager = RemoteJobManagerFactory::Instance().create("Inexistent"),
                      const std::runtime_error &);

    TSM_ASSERT_THROWS("create() with unsubscribed class should throw",
                      jobManager = RemoteJobManagerFactory::Instance().create("FakeJM"), const std::runtime_error &);
  }

  // minimal positive test
  void test_createFakeJM() {
    RemoteJobManagerFactory::Instance().subscribe<FakeJM>("FakeJM");
    // throws not found cause it is not in facilities.xml, but otherwise fine
    TSM_ASSERT_THROWS("create() with class name that is not defined in facilities should "
                      "throw",
                      jm = Mantid::API::RemoteJobManagerFactory::Instance().create("FakeJM"),
                      const Mantid::Kernel::Exception::NotFoundError &);
  }

  void test_exists() {
    // a bit of stress, unsubscribe after being subscribed with DECLARE_...,
    // then subscribe and the unsubscribe again
    TS_ASSERT_THROWS_NOTHING(Mantid::API::RemoteJobManagerFactory::Instance().unsubscribe("FakeJM"));
    TS_ASSERT_THROWS_NOTHING(Mantid::API::RemoteJobManagerFactory::Instance().subscribe<FakeJM>("FakeJM"));

    std::vector<std::string> keys = Mantid::API::RemoteJobManagerFactory::Instance().getKeys();
    size_t count = keys.size();

    TS_ASSERT_THROWS(Mantid::API::RemoteJobManagerFactory::Instance().subscribe<FakeJM>("FakeJM"),
                     const std::runtime_error &);
    TS_ASSERT_THROWS_NOTHING(Mantid::API::RemoteJobManagerFactory::Instance().unsubscribe("FakeJM"));
    TS_ASSERT_THROWS_NOTHING(Mantid::API::RemoteJobManagerFactory::Instance().subscribe<FakeJM>("FakeJM"));

    TS_ASSERT_THROWS_NOTHING(Mantid::API::RemoteJobManagerFactory::Instance().subscribe<FakeJMDeriv>("FakeJMDeriv"));
    TS_ASSERT_THROWS_NOTHING(Mantid::API::RemoteJobManagerFactory::Instance().subscribe<FakeJMDeriv>("FakeJM3"));

    TS_ASSERT(Mantid::API::RemoteJobManagerFactory::Instance().exists("FakeJM"));
    TS_ASSERT(Mantid::API::RemoteJobManagerFactory::Instance().exists("FakeJMDeriv"));
    TS_ASSERT(Mantid::API::RemoteJobManagerFactory::Instance().exists("FakeJM3"));

    // these are not in the facilities file
    TS_ASSERT_THROWS(jm = Mantid::API::RemoteJobManagerFactory::Instance().create("FakeJM"),
                     const std::runtime_error &);
    TS_ASSERT_THROWS(jm = Mantid::API::RemoteJobManagerFactory::Instance().create("FakeJMDeriv"),
                     const std::runtime_error &);

    keys = Mantid::API::RemoteJobManagerFactory::Instance().getKeys();
    size_t after = keys.size();

    TS_ASSERT_EQUALS(count + 2, after);

    TS_ASSERT_THROWS_NOTHING(Mantid::API::RemoteJobManagerFactory::Instance().unsubscribe("FakeJM"));
    TS_ASSERT_THROWS_NOTHING(Mantid::API::RemoteJobManagerFactory::Instance().unsubscribe("FakeJMDeriv"));
    TS_ASSERT_THROWS_NOTHING(Mantid::API::RemoteJobManagerFactory::Instance().unsubscribe("FakeJM3"));

    keys = Mantid::API::RemoteJobManagerFactory::Instance().getKeys();
    size_t newCount = keys.size();

    TS_ASSERT_EQUALS(after - 3, newCount);
  }

  // this must fail, resource not found in the current facility
  void test_createAlienResource() {
    // save facility, do this before any changes
    const Mantid::Kernel::FacilityInfo &prevFac = Mantid::Kernel::ConfigService::Instance().getFacility();

    Mantid::Kernel::ConfigService::Instance().setFacility(ISISFac);
    TSM_ASSERT_THROWS("create() with " + FermiName + "in a facility other than " + SNSFac + " should fail",
                      jm = Mantid::API::RemoteJobManagerFactory::Instance().create(FermiName),
                      const Mantid::Kernel::Exception::NotFoundError &);

    // restore facility, always do this at the end
    Mantid::Kernel::ConfigService::Instance().setFacility(prevFac.name());
  }

  // a simple positive test, meant to be moved to the job managers tests as
  // these are added
  void test_createRemoteManagers() {
    // save facility, do this before any changes
    const Mantid::Kernel::FacilityInfo &prevFac = Mantid::Kernel::ConfigService::Instance().getFacility();

    Mantid::Kernel::ConfigService::Instance().setFacility("SNS");
    TS_ASSERT(Mantid::API::RemoteJobManagerFactory::Instance().create("Fermi"));
    // restore facility, always do this at the end
    Mantid::Kernel::ConfigService::Instance().setFacility(prevFac.name());
  }

private:
  Mantid::API::IRemoteJobManager_sptr jm;

  static const std::string SNSFac;
  static const std::string ISISFac;
  static const std::string FermiName;
};

const std::string RemoteJobManagerFactoryTest::SNSFac = "SNS";
const std::string RemoteJobManagerFactoryTest::ISISFac = "ISIS";
const std::string RemoteJobManagerFactoryTest::FermiName = "Fermi";
