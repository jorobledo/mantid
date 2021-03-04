// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <utility>

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/NexusDescriptor.h"
#include "MantidDataHandling/EventWorkspaceCollection.h"
#include "MantidKernel/V3D.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {
namespace DataHandling {

/** LoadILLDiffraction : Loads ILL event files.

  @date 03/03/2021
*/
class MANTID_DATAHANDLING_DLL LoadILLEvent : public API::Algorithm{
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"LoadNexus"};
  }
  const std::string category() const override;
  const std::string summary() const override;
  LoadILLEvent();
private:
  void init() override;
  void exec() override;

  /// The workspace being filled out
  std::shared_ptr<EventWorkspaceCollection> m_ws;

};

} // namespace DataHandling
} // namespace Mantid
