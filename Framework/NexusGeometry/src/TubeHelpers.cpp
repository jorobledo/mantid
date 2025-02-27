// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidNexusGeometry/TubeHelpers.h"
#include "MantidGeometry/Objects/IObject.h"
#include <algorithm>
#include <iterator>
#include <tuple>
#include <vector>

namespace Mantid::NexusGeometry::TubeHelpers {

/**
 * Discover tubes based on pixel positions. Sort detector ids on the basis of
 * colinear positions.
 * @param detShape : Shape used for all detectors in tubes
 * @param detPositions : All detector positions across all tubes. Indexes match
 * detIDs.
 * @param detIDs : All detector ids across all tubes. Indexes match
 * detPositions.
 * @return. Collection of virtual tubes based on linearity of points.
 */
std::vector<detail::TubeBuilder> findAndSortTubes(const Mantid::Geometry::IObject &detShape, const Pixels &detPositions,
                                                  const std::vector<Mantid::detid_t> &detIDs) {
  std::vector<detail::TubeBuilder> tubes;
  tubes.emplace_back(detShape, detPositions.col(0), detIDs[0]);

  // Loop through all detectors and add to tubes
  for (size_t i = 1; i < detIDs.size(); ++i) {
    bool newEntry = true;
    for (auto t = tubes.rbegin(); t != tubes.rend(); ++t) {
      auto &tube = (*t);
      // Adding detector to existing tube
      if (tube.addDetectorIfCoLinear(detPositions.col(i), detIDs[i])) {
        newEntry = false;
        break;
      }
    }

    // Create a new tube if detector does not belong to any tubes
    if (newEntry)
      tubes.emplace_back(detShape, detPositions.col(i), detIDs[i]);
  }

  // Remove "tubes" with only 1 element
  tubes.erase(
      std::remove_if(tubes.begin(), tubes.end(), [](const detail::TubeBuilder &tube) { return tube.size() == 1; }),
      tubes.end());

  return tubes;
}

/**
 * Establish detector ids for any detector that is NOT part of the tubes
 *
 * @param tubes
 * @return vector of detector ids not part of tubes
 */
std::vector<Mantid::detid_t> notInTubes(const std::vector<detail::TubeBuilder> &tubes,
                                        std::vector<Mantid::detid_t> detIDs) {
  std::vector<Mantid::detid_t> used;
  for (const auto &tube : tubes) {
    for (const auto &id : tube.detIDs()) {
      used.emplace_back(id);
    }
  }
  std::vector<Mantid::detid_t> diff;
  std::sort(detIDs.begin(), detIDs.end());
  std::sort(used.begin(), used.end());
  std::set_difference(detIDs.begin(), detIDs.end(), used.begin(), used.end(), std::inserter(diff, diff.begin()));
  return diff;
}
} // namespace Mantid::NexusGeometry::TubeHelpers
