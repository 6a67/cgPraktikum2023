#include "meshlife/algorithms/mesh_automaton.h"

namespace meshlife
{

MeshAutomaton::MeshAutomaton(pmp::SurfaceMesh& mesh) : mesh_(mesh)
{
    if (!mesh_.has_face_property("f:state")) {
        state_ = mesh_.add_face_property<float>("f:state", 0.0f);
        last_state_ = mesh_.add_face_property<float>("f:last_state", 0.0f);
    }
}

void MeshAutomaton::allocate_needed_properties()
{
}

void MeshAutomaton::init_state_from_prop(pmp::FaceProperty<float>& prop)
{
    for (pmp::Face f : mesh_.faces())
    {
        state_[f] = prop[f];
        last_state_[f] = 0.0f;
    }
}

void MeshAutomaton::set_state(const pmp::Face& f, float value)
{
    state_[f] = value;
}

} // namespace meshlife
