#include "meshlife/algorithms/helpers.h"

namespace meshlife
{

namespace helpers
{

std::set<pmp::Face> get_neighbored_faces(pmp::SurfaceMesh& mesh, pmp::Face f)
{
    // set of neighbored faces
    std::set<pmp::Face> neighbored_faces;

    // get every vertex of the face
    std::vector<pmp::Vertex> vertices;
    for (auto v : mesh.vertices(f))
    {
        for (auto nf : mesh.faces(v))
        {
            if (nf != f)
            {
                neighbored_faces.insert(nf);
            }
        }
    }
    return neighbored_faces;
}

} // namespace helpers

} // namespace meshlife
