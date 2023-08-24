#include "meshlife/algorithms/mesh_gol.h"
#include "meshlife/algorithms/mesh_lenia.h"
#include "meshlife/paths.h"
#include "meshlife/visualization/viewer.h"
#include <omp.h>

std::filesystem::path assets_path;
std::filesystem::path shaders_path;

int main(int argc, char** argv)
{
    // omp_set_num_threads(4);

    init_paths();
    meshlife::Viewer window("Viewer", 800, 600);

    if (argc == 2)
        window.load_mesh(argv[1]);
#ifdef __EMSCRIPTEN__
    else
        window.load_mesh("input.off");
#endif

    window.set_automaton<meshlife::MeshLenia>();

    return window.run();
}
