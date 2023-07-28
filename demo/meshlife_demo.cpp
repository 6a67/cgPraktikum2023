#include "meshlife/visualization/viewer.h"

int main(int argc, char** argv)
{
    meshlife::Viewer window("Viewer", 800, 600);

    if (argc == 2)
        window.load_mesh(argv[1]);
#ifdef __EMSCRIPTEN__
    else
        window.load_mesh("input.off");
#endif

    return window.run();
}
