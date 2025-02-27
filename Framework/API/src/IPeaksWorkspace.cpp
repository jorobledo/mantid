// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/IPropertyManager.h"

namespace Mantid::API {

using namespace Kernel;

const std::string IPeaksWorkspace::toString() const {
  std::ostringstream os;
  os << ITableWorkspace::toString() << "\n" << ExperimentInfo::toString();
  if (convention == "Crystallography")
    os << "Crystallography: kf-ki";
  else
    os << "Inelastic: ki-kf";
  os << "\n";

  return os.str();
}

} // namespace Mantid::API

///\cond TEMPLATE

namespace Mantid::Kernel {

template <>
MANTID_API_DLL Mantid::API::IPeaksWorkspace_sptr
IPropertyManager::getValue<Mantid::API::IPeaksWorkspace_sptr>(const std::string &name) const {
  auto *prop = dynamic_cast<PropertyWithValue<Mantid::API::IPeaksWorkspace_sptr> *>(getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message =
        "Attempt to assign property " + name + " to incorrect type. Expected shared_ptr<PeaksWorkspace>.";
    throw std::runtime_error(message);
  }
}

template <>
MANTID_API_DLL Mantid::API::IPeaksWorkspace_const_sptr
IPropertyManager::getValue<Mantid::API::IPeaksWorkspace_const_sptr>(const std::string &name) const {
  auto *prop = dynamic_cast<PropertyWithValue<Mantid::API::IPeaksWorkspace_sptr> *>(getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message =
        "Attempt to assign property " + name + " to incorrect type. Expected const shared_ptr<PeaksWorkspace>.";
    throw std::runtime_error(message);
  }
}

} // namespace Mantid::Kernel

///\endcond TEMPLATE
