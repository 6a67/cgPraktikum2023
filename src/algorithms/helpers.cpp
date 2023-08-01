#include "meshlife/algorithms/helpers.h"

namespace meshlife
{

namespace helpers
{

std::set<pmp::Face> get_neighbored_faces(pmp::SurfaceMesh& mesh, pmp::Face f)
{
    // std::vector<pmp::Face> neighbored_faces;

    // This does not work, as it does not include diagonally neighbored faces and the game of life needs them
    // pmp::Halfedge start = mesh.halfedge(f);
    // pmp::Halfedge he = start;
    // do
    // {
    //     pmp::Face nf = mesh.face(mesh.opposite_halfedge(he));
    //     if (nf.is_valid())
    //     {
    //         neighbored_faces.push_back(nf);
    //     }
    //     he = mesh.next_halfedge(he);
    // } while (he != start);

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