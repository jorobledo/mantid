// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Surfaces/LineIntersectVisit.h"
#include "MantidGeometry/Surfaces/Cone.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/General.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidGeometry/Surfaces/Surface.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Strings.h"
#include <algorithm>

namespace Mantid::Geometry {

LineIntersectVisit::LineIntersectVisit(const Kernel::V3D &Pt, const Kernel::V3D &uVec)
    : ATrack(Pt, uVec)
/**
  Constructor
*/
{
  PtOut.reserve(2);
  DOut.reserve(2);
}

void LineIntersectVisit::Accept(const Surface &Surf)
/**
  Process an intersect track
  @param Surf :: Surface to use int line Interesect
*/
{
  (void)Surf; // Avoid compiler warning
  throw std::runtime_error("LineIntersectVisit::Accept Surface");
}

void LineIntersectVisit::Accept(const Quadratic &Surf)
/**
  Process an intersect track
  @param Surf :: Surface to use int line Interesect
*/
{
  ATrack.intersect(PtOut, Surf);
  procTrack();
}

void LineIntersectVisit::Accept(const Plane &Surf)
/**
  Process an intersect track
  @param Surf :: Surface to use int line Interesect
*/
{
  ATrack.intersect(PtOut, Surf);
  procTrack();
}

void LineIntersectVisit::Accept(const Cone &Surf)
/**
  Process an intersect track
  @param Surf :: Surface to use int line Interesect
*/
{
  ATrack.intersect(PtOut, Surf);
  procTrack();
}

void LineIntersectVisit::Accept(const Cylinder &Surf)
/**
  Process an intersect track
  @param Surf :: Surface to use int line Interesect
*/
{
  ATrack.intersect(PtOut, Surf);
  procTrack();
}

void LineIntersectVisit::Accept(const Sphere &Surf)
/**
  Process an intersect track
  @param Surf :: Surface to use int line Interesect
*/
{
  ATrack.intersect(PtOut, Surf);
  procTrack();
}

void LineIntersectVisit::Accept(const General &Surf)
/**
  Process an intersect track
  @param Surf :: Surface to use int line Interesect
*/
{
  ATrack.intersect(PtOut, Surf);
  procTrack();
}

void LineIntersectVisit::procTrack()
/**
  Sorts the PtOut and distances
  with a closes first order.
*/
{
  // Calculate the distances to the points
  DOut.resize(PtOut.size());
  using std::placeholders::_1;
  std::transform(PtOut.begin(), PtOut.end(), DOut.begin(), std::bind(&Kernel::V3D::distance, ATrack.getOrigin(), _1));
}

} // namespace Mantid::Geometry
