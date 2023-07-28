#pragma once

#include <pmp/visualization/mesh_viewer.h>

namespace meshlife
{

/// pmp::MeshViewer extension to handle visualization of implemented algorithms
class Viewer : public pmp::MeshViewer
{
  public:
    /// window constructor
    Viewer(const char* title, int width, int height);

  protected:
    /// thandles mouse button presses
    void mouse(int button, int action, int mods) override;

    /// handles keyboard events
    void keyboard(int key, int code, int action, int mod) override;

    /// provides custom GUI elements
    void process_imgui() override;
};

} // namespace meshlife
