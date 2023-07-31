#pragma once

#include "meshlife/algorithms/mesh_automaton.h"
#include "meshlife/algorithms/mesh_gol.h"
#include <pmp/visualization/mesh_viewer.h>

namespace meshlife
{

/// pmp::MeshViewer extension to handle visualization of implemented algorithms
class Viewer : public pmp::MeshViewer
{
  public:
    /// window constructor
    Viewer(const char* title, int width, int height);

    template <typename T>
    inline void set_automaton()
    {
        // TODO: Free in destructor
        automaton = new T(mesh_);
    }

    void set_mesh_properties();

  protected:
    /// thandles mouse button presses
    void mouse(int button, int action, int mods) override;

    /// handles keyboard events
    void keyboard(int key, int code, int action, int mod) override;

    /// provides custom GUI elements
    void process_imgui() override;

    void do_processing() override;

    bool find_face(int x, int y, pmp::Face& face);

    void set_face_color(pmp::Face& face, pmp::Color color);
    void set_face_gol_alive(pmp::Face& face, bool alive);

  private:
    MeshAutomaton* automaton = nullptr;
    bool a_gol = false;
};

} // namespace meshlife
