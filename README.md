# meshlife — Life on Meshes
In this practical course on mesh processing and computer graphics you will try to craft some pretty, biology-inspired visualizations, maybe even something like art — but purely algorithmically!

The course will be focusing mainly on implementing abstract procedural models for biological growth and creating visually pleasing animations of the time-development of these systems.

## Overview

As a starting point, you will familiarize yourselves with navigation on polygonal meshes by implementing *Conway's Game of Life*, a famous example of a 2D Cellular Automaton, on quad meshes (i.e. meshes formed by quadrangles instead of triangles). Step by step, we will generalize the classic *GoL* algorithm to also work on arbitrary meshes (e.g. made from triangles, or hexagons, or a mix of those).

From there on, work will proceed more freely, allowing you to give priority to:
- extending work on cellular automata by adapting more complex systems like *SmoothLife* or *Lenia* (demo [here](https://www.youtube.com/watch?v=6kiBYjvyojQ&t=100s))
- exploring mesh and geometry modification, e.g. by implementing a variant of *cellular forms* (demo [here](https://www.youtube.com/watch?v=Yy-ye_x3_zA))
- exploring mesh generation, e.g. by implementing marching cubes (short tutorial [here](http://paulbourke.net/geometry/polygonise/)) to extract a mesh from a 3D grid-based cellular automaton
- enhancing the rendering and animation of the above by combining the above with common techniques such as
    * texture/normal/bump/parallax/... mapping
    * shadows & ambient occlusion
    * transparency
    * reflection
    * depth-of-field
    * skybox
    * shader-only geometry (like fractals, metaballs)
    * etc.
* any other ideas you might come up with :)

## Preliminaries

Before diving right in, you should make yourself familiar (at least to a basic extent) with the tools and software we use:
- git & gitlab
- Ubuntu (or linux-based operating systems in general)
- VSCode (or your Editor/IDE of choice)
- cmake
- C++
- the pmp library ("polygon mesh processing")
    * this is probably the most important, because most of our work will be based on this library
- the ImGui library

The [quick start guide](GUIDE.md) will try to provide you with a short overview of these, highlighting aspects relevant for this practical course. Apart from that, during contact hours I (read as: your practical course instructor) will always be ready and happy to help :)

# Profiling
Tools
- perf
- hotspot: https://github.com/KDAB/hotspot

Command:
```bash
perf record --call-graph=dwarf ./bin/RelWithDebInfo/meshlife_demo
hotspot
```
hotspot takes the perf.data in the same directory automatically

# Fractals
https://www.youtube.com/watch?v=svLzmFuSBhk
https://www.youtube.com/watch?v=BNZtUB7yhX4
https://www.youtube.com/watch?v=PGtv-dBi2wE
https://www.youtube.com/watch?v=Cp5WWtMoeKg


# Our changes to external code

c23b003 only formatted the external code with clang-format, afterwards we changed stuff there.
To see all changes to the external code since then use:
(c2eb003~1 is the commit before the formatting, so the formatting changes are not included)
```git diff c2eb003..main -- ./extern```

To see all changes before the formatting use:
```git diff 5bc325a..c2eb003~1 -- ./extern```

