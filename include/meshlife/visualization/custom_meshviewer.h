#pragma once

#include "custom_renderer.h"
#include "pmp/surface_mesh.h"
#include "pmp/visualization/trackball_viewer.h"
namespace meshlife
{

class CustomMeshViewer : public pmp::TrackballViewer
{
  public:
    //! constructor
    CustomMeshViewer(const char* title, int width, int height, bool showgui = true);

    //! destructor
    ~CustomMeshViewer() override = default;

    //! load a mesh from file \p filename
    void load_mesh(const char* filename);

    //! update mesh normals and all buffers for OpenGL rendering.  call this
    //! function whenever you change either the vertex positions or the
    //! triangulation of the mesh
    void update_mesh();

    //! draw the scene in different draw modes
    void draw(const std::string& draw_mode) override;

    //! handle ImGUI interface
    void process_imgui() override;

    //! this function handles keyboard events
    void keyboard(int key, int code, int action, int mod) override;

    //! get vertex closest to 3D position under the mouse cursor
    pmp::Vertex pick_vertex(int x, int y);

  protected:
    pmp::SurfaceMesh mesh_;

    CustomRenderer renderer_;

    std::string filename_; //!< the current file

    float crease_angle_;
};

} // namespace meshlife
