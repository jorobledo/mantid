// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadILLEvent.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"

#include <H5Cpp.h>
#include <lstdpp128/lstdpp.h>
#include <nexus/napi.h>

namespace Mantid {
namespace DataHandling {

using namespace API;
using namespace Geometry;
using namespace H5;
using namespace Kernel;
using namespace NeXus;
using Types::Core::DateAndTime;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadILLEvent)

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadILLEvent::name() const { return "LoadILLEvent"; }

void LoadILLEvent::init() {
  declareProperty(std::make_unique<FileProperty>("LSTFilename", "",
                                                 FileProperty::Load, ".lst"),
                  "Path to the lst file");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "the output");
}

void LoadILLEvent::exec() {
  m_ws = std::make_shared<EventWorkspaceCollection>();

  lstdpp128::Reader reader(1024);
  bool success = reader.open(getPropertyValue("LSTFilename"));
  // Exit in case of error.
  if (!success) {
    std::cout << "Unable to read file" << std::endl;
  }

  std::cout << lstdpp128::listModeContext << std::endl;
}

} // namespace DataHandling
} // namespace Mantid
