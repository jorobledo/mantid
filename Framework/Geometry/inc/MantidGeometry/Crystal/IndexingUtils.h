// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/* File: Indexing_Utils.h */

#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace Geometry {
/**
    @class IndexingUtils

    This class contains static utility methods for indexing peaks and
    finding the UB matrix.

    @author Dennis Mikkelson
    @date   2011-06-14
 */

class MANTID_GEOMETRY_DLL IndexingUtils {
public:
  /// Find the UB matrix that most nearly indexes the specified qxyz values
  /// given the lattice parameters
  static double Find_UB(Kernel::DblMatrix &UB, const std::vector<Kernel::V3D> &q_vectors, OrientedLattice &lattice,
                        double required_tolerance, int base_index, size_t num_initial, double degrees_per_step,
                        bool fixAll = false, int iterations = 1);

  /// Find the UB matrix that most nearly indexes the specified qxyz values
  /// given the range of possible real space unit cell edge lengths.
  static double Find_UB(Kernel::DblMatrix &UB, const std::vector<Kernel::V3D> &q_vectors, double min_d, double max_d,
                        double required_tolerance, int base_index, size_t num_initial, double degrees_per_step);

  /// Find the UB matrix that most nearly indexes the specified qxyz values
  /// using FFTs, given the range of possible real space unit cell edge lengths
  static double Find_UB(Kernel::DblMatrix &UB, const std::vector<Kernel::V3D> &q_vectors, double min_d, double max_d,
                        double required_tolerance, double degrees_per_step, int iterations = 4);

  /// Find the UB matrix that most nearly maps hkl to qxyz for 3 or more peaks
  static double Optimize_UB(Kernel::DblMatrix &UB, const std::vector<Kernel::V3D> &hkl_vectors,
                            const std::vector<Kernel::V3D> &q_vectors, std::vector<double> &sigabc);

  /// Find the UB matrix that most nearly maps hkl to qxyz for 3 or more peaks
  static double Optimize_UB(Kernel::DblMatrix &UB, const std::vector<Kernel::V3D> &hkl_vectors,
                            const std::vector<Kernel::V3D> &q_vectors);

  static double Optimize_6dUB(Kernel::DblMatrix &UB, Kernel::DblMatrix &ModUB,
                              const std::vector<Kernel::V3D> &hkl_vectors, const std::vector<Kernel::V3D> &mnp_vectors,
                              const int &ModDim, const std::vector<Kernel::V3D> &q_vectors, std::vector<double> &sigabc,
                              std::vector<double> &sigq);

  static double Optimize_6dUB(Kernel::DblMatrix &UB, Kernel::DblMatrix &ModUB,
                              const std::vector<Kernel::V3D> &hkl_vectors, const std::vector<Kernel::V3D> &mnp_vectors,
                              const int &ModDim, const std::vector<Kernel::V3D> &q_vectors);

  /// Find the vector that best corresponds to plane normal, given 1-D indices
  static double Optimize_Direction(Kernel::V3D &best_vec, const std::vector<int> &index_values,
                                   const std::vector<Kernel::V3D> &q_vectors);

  /// Scan rotations to find UB that indexes peaks given lattice parameters
  static double ScanFor_UB(Kernel::DblMatrix &UB, const std::vector<Kernel::V3D> &q_vectors, const UnitCell &cell,
                           double degrees_per_step, double required_tolerance);

  /// Get list of possible directions and lengths for real space unit cell
  static size_t ScanFor_Directions(std::vector<Kernel::V3D> &directions, const std::vector<Kernel::V3D> &q_vectors,
                                   double min_d, double max_d, double required_tolerance, double degrees_per_step);

  /// Use FFT to get list of possible directions and lengths for real space
  /// unit cell
  static size_t FFTScanFor_Directions(std::vector<Kernel::V3D> &directions, const std::vector<Kernel::V3D> &q_vectors,
                                      double min_d, double max_d, double required_tolerance, double degrees_per_step);

  /// Get the magnitude of the FFT of the projections of the q_vectors on
  /// the current direction vector.
  static double GetMagFFT(const std::vector<Kernel::V3D> &q_vectors, const Kernel::V3D &current_dir, const size_t N,
                          double projections[], double index_factor, double magnitude_fft[]);

  /// Get the location of the first maximum (beyond the DC term) in the |FFT|
  /// that exceeds the specified threshold.
  static double GetFirstMaxIndex(const double magnitude_fft[], size_t N, double threshold);

  /// Try to form a UB matrix from three vectors chosen from list of possible
  /// a,b,c directions, starting with the a vector at the given index.
  static bool FormUB_From_abc_Vectors(Kernel::DblMatrix &UB, const std::vector<Kernel::V3D> &directions, size_t a_index,
                                      double min_d, double max_d);

  /// Form a UB matrix by choosing three vectors from list of possible a,b,c
  /// to maximize the number of peaks indexed and minimize cell volume.
  static bool FormUB_From_abc_Vectors(Kernel::DblMatrix &UB, const std::vector<Kernel::V3D> &directions,
                                      const std::vector<Kernel::V3D> &q_vectors, double req_tolerance, double min_vol);

  /// Get the vector in the direction of "c" given other unit cell information
  static Kernel::V3D makeCDir(const Kernel::V3D &a_dir, const Kernel::V3D &b_dir, const double c, const double cosAlpha,
                              const double cosBeta, const double cosGamma, const double sinGamma);

  /// Construct a sublist of the specified list of a,b,c directions, by
  /// removing all directions that seem to be duplicates.
  static void DiscardDuplicates(std::vector<Kernel::V3D> &new_list, std::vector<Kernel::V3D> &directions,
                                const std::vector<Kernel::V3D> &q_vectors, double required_tolerance, double len_tol,
                                double ang_tol);

  /// Round all the components of a HKL objects to the nearest integer
  static void RoundHKL(Kernel::V3D &hkl);

  /// Round all the components of a list of V3D objects, to the nearest integer
  static void RoundHKLs(std::vector<Kernel::V3D> &hkl_list);

  /// Check is hkl is within tolerance of integer (h,k,l) non-zero values
  static bool ValidIndex(const Kernel::V3D &hkl, double tolerance);

  /// Find number of valid HKLs and average error, in list of HKLs
  static int NumberOfValidIndexes(const std::vector<Kernel::V3D> &hkls, double tolerance, double &average_error);

  /// Find the average indexing error for UB with the specified q's and hkls
  static double IndexingError(const Kernel::DblMatrix &UB, const std::vector<Kernel::V3D> &hkls,
                              const std::vector<Kernel::V3D> &q_vectors);

  /// Check that the specified UB is reasonable for an orientation matrix
  static bool CheckUB(const Kernel::DblMatrix &UB);

  /// Calculate the number of Q vectors that are mapped to integer indices by UB
  static int NumberIndexed(const Kernel::DblMatrix &UB, const std::vector<Kernel::V3D> &q_vectors, double tolerance);

  /// Calculate the number of Q vectors that map to an integer index in one
  /// direction
  static int NumberIndexed_1D(const Kernel::V3D &direction, const std::vector<Kernel::V3D> &q_vectors,
                              double tolerance);

  /// Calculate the number of Q vectors that map to integer indices
  /// simutlaneously in three directions
  static int NumberIndexed_3D(const Kernel::V3D &a_dir, const Kernel::V3D &b_dir, const Kernel::V3D &c_dir,
                              const std::vector<Kernel::V3D> &q_vectors, double tolerance);

  /// Given a UB, get list of Miller indices for specifed Qs and tolerance
  static int CalculateMillerIndices(const Kernel::DblMatrix &UB, const std::vector<Kernel::V3D> &q_vectors,
                                    double tolerance, std::vector<Kernel::V3D> &miller_indices, double &ave_error);

  /// Given a UB, calculate the miller indices for given q vector
  static bool CalculateMillerIndices(const Kernel::DblMatrix &inverseUB, const Kernel::V3D &q_vector, double tolerance,
                                     Kernel::V3D &miller_indices);

  /// Given a UB, calculate the miller indices for given q vector
  static Kernel::V3D CalculateMillerIndices(const Kernel::DblMatrix &inverseUB, const Kernel::V3D &q_vector);

  /// Get lists of indices and Qs for peaks indexed in the specified direction
  static int GetIndexedPeaks_1D(const Kernel::V3D &direction, const std::vector<Kernel::V3D> &q_vectors,
                                double required_tolerance, std::vector<int> &index_vals,
                                std::vector<Kernel::V3D> &indexed_qs, double &fit_error);

  /// Get lists of indices and Qs for peaks indexed in three given directions
  static int GetIndexedPeaks_3D(const Kernel::V3D &direction_1, const Kernel::V3D &direction_2,
                                const Kernel::V3D &direction_3, const std::vector<Kernel::V3D> &q_vectors,
                                double required_tolerance, std::vector<Kernel::V3D> &miller_indices,
                                std::vector<Kernel::V3D> &indexed_qs, double &fit_error);

  /// Get lists of indices and Qs for peaks indexed by the specified UB matrix
  static int GetIndexedPeaks(const Kernel::DblMatrix &UB, const std::vector<Kernel::V3D> &q_vectors,
                             double required_tolerance, std::vector<Kernel::V3D> &miller_indices,
                             std::vector<Kernel::V3D> &indexed_qs, double &fit_error);

  /// Make list of direction vectors uniformly distributed over a hemisphere
  static std::vector<Kernel::V3D> MakeHemisphereDirections(int n_steps);

  /// Make list of the circle of direction vectors that form a fixed angle
  /// with the specified axis
  static std::vector<Kernel::V3D> MakeCircleDirections(int n_steps, const Kernel::V3D &axis, double angle_degrees);

  /// Choose the direction in a list of directions, that is most nearly
  /// perpendicular to planes with the specified spacing in reciprocal space.
  static int SelectDirection(Kernel::V3D &best_direction, const std::vector<Kernel::V3D> &q_vectors,
                             const std::vector<Kernel::V3D> &direction_list, double plane_spacing,
                             double required_tolerance);

  /// Get the lattice parameters for the specified orientation matrix
  static bool GetLatticeParameters(const Kernel::DblMatrix &UB, std::vector<double> &lattice_par);

  static int GetModulationVectors(const Kernel::DblMatrix &UB, const Kernel::DblMatrix &ModUB, Kernel::V3D &ModVec1,
                                  Kernel::V3D &ModVec2, Kernel::V3D &ModVec3);

  static bool GetModulationVector(const Kernel::DblMatrix &UB, const Kernel::DblMatrix &ModUB, Kernel::V3D &ModVec,
                                  int &j);

  /// Get a formatted string listing the lattice parameters and cell volume
  static std::string GetLatticeParameterString(const Kernel::DblMatrix &UB);

private:
  /// Static reference to the logger class
  static Mantid::Kernel::Logger &g_Log;
};

} // namespace Geometry
} // namespace Mantid
