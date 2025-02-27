// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/NexusDescriptor.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {
namespace DataHandling {
/**
 Loads an ILL IN4/5/6/Panther NeXus file into a Mantid workspace.
 */
class DLLExport LoadILLTOF2 : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  /// Constructor
  LoadILLTOF2();
  /// Algorithm's name
  const std::string name() const override { return "LoadILLTOF"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Loads an ILL TOF NeXus file."; }

  /// Algorithm's version
  int version() const override { return 2; }
  const std::vector<std::string> seeAlso() const override { return {"LoadNexus"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Nexus;ILL\\Direct"; }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

private:
  // Initialisation code
  void init() override;
  // Execution code
  void exec() override;

  void loadInstrumentDetails(NeXus::NXEntry &);
  std::vector<std::vector<int>> getMonitorInfo(NeXus::NXEntry &firstEntry);
  void initWorkSpace(NeXus::NXEntry &entry, const std::vector<std::vector<int>> &);
  void initInstrumentSpecific();
  void addAllNexusFieldsAsProperties(const std::string &filename);
  void addEnergyToRun();
  void addFacility();
  void addPulseInterval();

  void loadTimeDetails(NeXus::NXEntry &entry);
  void loadDataIntoTheWorkSpace(NeXus::NXEntry &entry, const std::vector<std::vector<int>> &, bool convertToTOF);
  void loadSpectra(size_t &spec, const size_t numberOfTubes, const std::vector<Mantid::detid_t> &detectorIDs,
                   const NeXus::NXInt &data, Mantid::API::Progress &progress);

  void runLoadInstrument();

  /// Calculate error for y
  static double calculateError(double in) { return sqrt(in); }

  API::MatrixWorkspace_sptr m_localWorkspace;

  std::string m_instrumentName; ///< Name of the instrument
  std::string m_instrumentPath; ///< Name of the instrument path

  // Variables describing the data in the detector
  size_t m_numberOfTubes;         // number of tubes - X
  size_t m_numberOfPixelsPerTube; // number of pixels per tube - Y
  size_t m_numberOfChannels;      // time channels - Z
  size_t m_numberOfHistograms;

  /* Values parsed from the nexus file */
  double m_wavelength;
  double m_channelWidth;
  double m_timeOfFlightDelay;
  std::string m_monitorName;

  LoadHelper m_loader;
};

} // namespace DataHandling
} // namespace Mantid
