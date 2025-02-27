// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidRemoteAlgorithms/Authenticate2.h"
#include "MantidAPI/RemoteJobManagerFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/MaskedProperty.h"

namespace Mantid::RemoteAlgorithms {

// Register the algorithm into the Algorithm Factory
DECLARE_ALGORITHM(Authenticate2)

using namespace Mantid::Kernel;
// using namespace Mantid::API;
// using namespace Mantid::Geometry;

// A reference to the logger is provided by the base class, it is called g_log.

void Authenticate2::init() {
  // Unlike most algorithms, this wone doesn't deal with workspaces....

  auto requireValue = std::make_shared<MandatoryValidator<std::string>>();

  // Compute Resources
  std::vector<std::string> computes = Mantid::Kernel::ConfigService::Instance().getFacility().computeResources();
  declareProperty("ComputeResource", "", std::make_shared<StringListValidator>(computes),
                  "The remote computer to authenticate to", Direction::Input);

  // Say who we are (or at least, who we want to execute the remote python code)
  declareProperty("UserName", "", requireValue, "Name of the user to authenticate as", Direction::Input);

  // Password doesn't get echoed to the screen...
  declareProperty(std::make_unique<MaskedProperty<std::string>>("Password", "", requireValue, Direction::Input),
                  "The password associated with the specified user");
}

void Authenticate2::exec() {

  const std::string comp = getPropertyValue("ComputeResource");
  Mantid::API::IRemoteJobManager_sptr jobManager = Mantid::API::RemoteJobManagerFactory::Instance().create(comp);

  const std::string user = getPropertyValue("UserName");
  jobManager->authenticate(user, getPropertyValue("Password"));

  g_log.information() << "Authenticate as user " << user << " in the compute resource " << comp << '\n';
}

} // namespace Mantid::RemoteAlgorithms
