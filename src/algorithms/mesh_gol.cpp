#include "meshlife/algorithms/mesh_gol.h"
#include "meshlife/algorithms/helpers.h"
#include "meshlife/algorithms/mesh_automaton.h"

#include <pmp/surface_mesh.h>

namespace meshlife
{

MeshGOL::MeshGOL(pmp::SurfaceMesh& mesh) : MeshAutomaton(mesh){};

MeshGOL::~MeshGOL(){};

void MeshGOL::init_state_random()
{
    // make random faces alive
    for (pmp::Face f : mesh_.faces())
    {
        state_[f] = (rand() % 2);
    }
};

void MeshGOL::update_state(int num_steps)
{
    // conway's game of life for the faces of the mesh

    for (int i = 0; i < num_steps; i++)
    {
        // make copy of state_ to last_state_
        for (pmp::Face f : mesh_.faces())
        {
            last_state_[f] = state_[f];
        }

        for (auto f : mesh_.faces())
        {
            // get neighbored faces
            auto neighbored_faces = helpers::get_neighbored_faces(mesh_, f);

            // count number of alive neighbored faces
            int num_alive = 0;
            for (auto nf : neighbored_faces)
            {
                if (last_state_[nf] == 1.0f)
                {
                    num_alive++;
                }
            }

            // Any live cell with two or three live neighbours survives
            if (last_state_[f] == 1.0f && num_alive >= p_lower_threshold_ && num_alive <= p_upper_threshold_)
            {
                state_[f] = 1.0f;
            }
            // Any dead cell with three live neighbours becomes a live cell
            else if (last_state_[f] == 0.0f && num_alive == p_upper_threshold_)
            {
                state_[f] = 1.0f;
            }
            // All other live cells die in the next generation. Similarly, all other dead cells stay dead
            else
            {
                state_[f] = 0.0f;
            }
        }
    }
}

} // namespace meshlife
