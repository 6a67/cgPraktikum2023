#include "meshlife/algorithms/mesh_automaton.h"

namespace meshlife
{

MeshAutomaton::MeshAutomaton(pmp::SurfaceMesh& mesh) : mesh_(mesh)
{
}

void MeshAutomaton::allocate_needed_properties()
{
    // If a property with this type and name did not exist before, it is allocated with default value 0.0f
    state_ = mesh_.face_property<float>("f:cellstate", 0.0f);
    last_state_ = mesh_.face_property<float>("f:cellstatelast", 0.0f);

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

} // namespace meshlife
