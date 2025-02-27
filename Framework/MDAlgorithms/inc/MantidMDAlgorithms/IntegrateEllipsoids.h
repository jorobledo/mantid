// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidMDAlgorithms/IntegrateQLabEvents.h"
#include "MantidMDAlgorithms/MDTransfInterface.h"
#include "MantidMDAlgorithms/MDWSDescription.h"
#include "MantidMDAlgorithms/UnitsConversionHelper.h"

namespace Mantid {
namespace Geometry {
class DetectorInfo;
}
namespace MDAlgorithms {
// Note: this relies on several MD specific interfaces, so it needs to be defined
// in MDAlgorithms despite it not actually operating on MDWorkspaces

class DLLExport IntegrateEllipsoids : public API::Algorithm {
public:
  const std::string name() const override { return "IntegrateEllipsoids"; }

  const std::string summary() const override {
    return "Integrate Single Crystal Diffraction Bragg peaks using 3D "
           "ellipsoids.";
  }

  int version() const override { return 1; }

  const std::vector<std::string> seeAlso() const override { return {"IntegrateEllipsoidsTwoStep"}; }
  const std::string category() const override { return "Crystal\\Integration"; }

private:
  /// Initialize the algorithm's properties
  void init() override;
  /// Execute the algorithm
  void exec() override;
  /// Private validator for inputs
  std::map<std::string, std::string> validateInputs() override;

  /**
   * @brief create a list of SlimEvent objects from an events workspace
   * @param integrator : integrator object on the list is accumulated
   * @param prog : progress object
   * @param wksp : input EventWorkspace
   */
  void qListFromEventWS(IntegrateQLabEvents &integrator, API::Progress &prog, DataObjects::EventWorkspace_sptr &wksp);

  /**
   * @brief create a list of SlimEvent objects from a histogram workspace
   * @param integrator : integrator object on which the list is accumulated
   * @param prog : progress object
   * @param wksp : input Workspace2D
   */
  void qListFromHistoWS(IntegrateQLabEvents &integrator, API::Progress &prog, DataObjects::Workspace2D_sptr &wksp);

  /// Calculate if this Q is on a detector
  void calculateE1(const Geometry::DetectorInfo &detectorInfo);

  /// Write the profiles of each  principle axis to the output workspace with fixed name
  void outputAxisProfiles(std::vector<double> &principalaxis1, std::vector<double> &principalaxis2,
                          std::vector<double> &principalaxis3, const double &cutoffIsigI, const int &numSigmas,
                          std::vector<DataObjects::Peak> &peaks, IntegrateQLabEvents &integrator);

  /// Write Axis profile to a MatrixWorkspace (Workspace2D)
  void outputProfileWS(const std::vector<double> &principalaxis1, const std::vector<double> &principalaxis2,
                       const std::vector<double> &principalaxis3, const std::string &wsname);

  /// Integrate peaks again with cutoff value of I/Sig(I)
  void integratePeaksCutoffISigI(const double &meanMax, const double &stdMax, std::vector<double> &principalaxis1,
                                 std::vector<double> &principalaxis2, std::vector<double> &principalaxis3,
                                 const int &numSigmas, std::vector<DataObjects::Peak> &peaks,
                                 IntegrateQLabEvents &integrator_satellite);

  void runMaskDetectors(const Mantid::DataObjects::PeaksWorkspace_sptr &peakWS, const std::string &property,
                        const std::string &values);

  /// Pair all Bragg peaks with their related satellite peaks
  void pairBraggSatellitePeaks(const size_t &n_peaks, std::vector<DataObjects::Peak> &peaks,
                               std::map<size_t, std::vector<DataObjects::Peak *>> &satellitePeakMap,
                               std::vector<size_t> &satellitePeaks);

  /// Remove shared background from each satellite peak
  void removeSharedBackground(std::map<size_t, std::vector<DataObjects::Peak *>> &satellitePeakMap,
                              std::map<size_t, std::pair<double, double>> &cachedBraggBackground);

  /// save for all detector pixels
  std::vector<Kernel::V3D> E1Vec;

  MDWSDescription m_targWSDescr;

  /// peak radius for Bragg peaks
  double m_braggPeakRadius;
  /// peak radius for satellite peaks
  double m_satellitePeakRadius;

  /**
   * @brief Initialize the output information for the MD conversion framework.
   * @param wksp : The workspace to get information from.
   */
  void initTargetWSDescr(API::MatrixWorkspace_sptr &wksp);
};

} // namespace MDAlgorithms
} // namespace Mantid
