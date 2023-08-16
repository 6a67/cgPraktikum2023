#pragma once

#include <pmp/surface_mesh.h>
#include <set>

namespace meshlife
{

namespace helpers
{

/// Returns a set of neigbored faces of face f
std::set<pmp::Face> get_neighbored_faces(pmp::SurfaceMesh& mesh, pmp::Face f);

} // namespace helpers

} // namespace meshlife
