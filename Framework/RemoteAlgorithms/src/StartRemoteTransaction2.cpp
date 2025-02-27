// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidRemoteAlgorithms/StartRemoteTransaction2.h"
#include "MantidAPI/RemoteJobManagerFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid::RemoteAlgorithms {

// Register the algorithm into the Algorithm Factory
DECLARE_ALGORITHM(StartRemoteTransaction2)

using namespace Mantid::Kernel;

// A reference to the logger is provided by the base class, it is called g_log.

void StartRemoteTransaction2::init() {
  // Compute Resources
  std::vector<std::string> computes = Mantid::Kernel::ConfigService::Instance().getFacility().computeResources();
  declareProperty("ComputeResource", "", std::make_shared<StringListValidator>(computes),
                  "The name of the remote computer where the new transaction "
                  "will be created",
                  Direction::Input);

  // output property
  declareProperty("TransactionID", std::string(""), "The ID of the new transaction", Direction::Output);
}

void StartRemoteTransaction2::exec() {

  Mantid::API::IRemoteJobManager_sptr jm =
      Mantid::API::RemoteJobManagerFactory::Instance().create(getPropertyValue("ComputeResource"));

  std::string tid = jm->startRemoteTransaction();

  setPropertyValue("TransactionID", tid);
  g_log.information() << "Transaction ID " << tid << " started.\n";
}

} // namespace Mantid::RemoteAlgorithms
