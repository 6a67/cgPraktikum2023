#include "meshlife/algorithms/mesh_automaton.h"

namespace meshlife
{

MeshAutomaton::MeshAutomaton(pmp::SurfaceMesh& mesh) : mesh_(mesh)
{
}

void MeshAutomaton::allocate_needed_properties()
{
    // If a property with this type and name did not exist before, it is allocated with default value 0.0f
    if (!mesh_.has_face_property("f:gol_state"))
        state_ = mesh_.add_face_property<float>("f:gol_state", 0.0f);

    // TODO: handle last_state_
    // last_state_ = mesh_.face_property<float>("f:cellstatelast", 0.0f);

    // Deallocating is not needed later, as it is done by the mesh's internal property manager
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
