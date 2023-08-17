#pragma once

#include <map>

#include "custom_renderer.h"
#include "meshlife/shadertype.h"
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

    // Notifies the viewer that a shader reload is required
    // this is used so the file watcher thread can notify the viewer thread
    // that a shader file has changed (otherwise it might reload mid-draw which causes a crash sometimes)
    void notify_shader_reload_required(ShaderType shader_typ);

  protected:
    pmp::SurfaceMesh mesh_;

    float mesh_size_uniform_ = 1.0f;
    float mesh_size_x_ = 1.0f;
    float mesh_size_y_ = 1.0f;
    float mesh_size_z_ = 1.0f;

    CustomRenderer renderer_;

    std::string filename_; //!< the current file

    float crease_angle_;

    // maps the shader type to a boolean indicating whether it needs to be reloaded before the next draw call
    std::map<ShaderType, bool> shader_reload_required_map_;
};

} // namespace meshlife
