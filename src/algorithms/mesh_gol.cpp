#include "meshlife/algorithms/mesh_gol.h"
#include "meshlife/algorithms/mesh_automaton.h"

#include <pmp/surface_mesh.h>

namespace meshlife
{

MeshGOL::MeshGOL(pmp::SurfaceMesh& mesh) : MeshAutomaton(mesh)
{
    allocate_needed_properties();
};

MeshGOL::~MeshGOL(){};

void MeshGOL::init_state_random()
{
    std::cout << "NOT IMPLEMENTED\n";
};

void MeshGOL::update_state(int num_steps)
{
}

} // namespace meshlife
