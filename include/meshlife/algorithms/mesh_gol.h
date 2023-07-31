#pragma once

#include "mesh_automaton.h"
#include <pmp/surface_mesh.h>

namespace meshlife
{

/// base class to handle cellular automata on meshes
class MeshGOL : public MeshAutomaton
{
  public:
    /// Create a base automaton algorithm instance working on \p mesh
    MeshGOL(pmp::SurfaceMesh& mesh);

    ~MeshGOL();

    void update_state(int num_steps) override;

    void init_state_random() override;
    /// Allocates the properties to store current and last state.
    /// Must be called before initializing the state
};

} // namespace meshlife
