#include "meshlife/visualization/custom_meshviewer.h"
#include "imgui.h"
#include "pmp/algorithms/utilities.h"
#include "pmp/bounding_box.h"
#include "pmp/io/io.h"
#include "pmp/visualization/trackball_viewer.h"
#include "stb_image.h"
#include <unistd.h>

namespace meshlife
{

CustomMeshViewer::CustomMeshViewer(const char* title, int width, int height, bool showgui)
    : pmp::TrackballViewer(title, width, height, showgui), renderer_(mesh_, window_)
{
    // setup draw modes
    clear_draw_modes();
    add_draw_mode("Fractal Mode With Mesh");
    add_draw_mode("Skybox only");
    add_draw_mode("Skybox with model");
    add_draw_mode("Custom Shader");
    add_draw_mode("Reflective Sphere");

    // load icon as GLFW image
    GLFWimage icon;
    icon.height = 256;
    icon.width = 256;
    // load image as char array
    icon.pixels = stbi_load("../icon.png", &icon.width, &icon.height, 0, 4);

    glfwSetWindowIcon(window_, 1, &icon);
}

//! load a mesh from file \p filename
void CustomMeshViewer::load_mesh(const char* filename)
{
    // load mesh
    try
    {
        pmp::read(mesh_, filename);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        throw;
    }

    // update scene center and bounds
    pmp::BoundingBox bb = pmp::bounds(mesh_);
    set_scene((pmp::vec3)bb.center(), 0.5 * bb.size());

    // compute face & vertex normals, update face indices
    update_mesh();

    filename_ = filename;

    // TODO: Figure out if and how we need to use this. Seems useful?
    // renderer_.set_crease_angle(crease_angle_);
}

void CustomMeshViewer::update_mesh()
{
    // update scene center and radius, but don't update camera view
    pmp::BoundingBox bb = bounds(mesh_);
    center_ = (pmp::vec3)bb.center();
    center_ = pmp::vec3(center_[0] * mesh_size_x_, center_[1] * mesh_size_y_, center_[2] * mesh_size_z_);
    radius_ = 0.5f * bb.size();

    // re-compute face and vertex normals
    renderer_.update_opengl_buffers();
}

//! draw the scene in different draw modes
void CustomMeshViewer::draw(const std::string& draw_mode)
{
    // Reload shaders if necessary
    if (shader_reload_required_map_[ShaderType::SimpleVert] || shader_reload_required_map_[ShaderType::SimpleFrag])
        renderer_.load_simple_shader();

    if (shader_reload_required_map_[ShaderType::SkyboxVert] || shader_reload_required_map_[ShaderType::SkyboxFrag])
        renderer_.load_skybox_shader();

    if (shader_reload_required_map_[ShaderType::ReflectiveSphereFrag]
        || shader_reload_required_map_[ShaderType::ReflectiveSphereVert])
        renderer_.load_reflective_sphere_shader();

    if (shader_reload_required_map_[ShaderType::PhongVert] || shader_reload_required_map_[ShaderType::PhongFrag])
        renderer_.load_phong_shader();

    // draw mesh
    renderer_.draw(projection_matrix_, get_modelview_matrix(), draw_mode);
}

//! handle ImGUI interface
void CustomMeshViewer::process_imgui()
{
    if (ImGui::CollapsingHeader("Mesh Info", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // output mesh statistics
        ImGui::BulletText("%d vertices", (int)mesh_.n_vertices());
        ImGui::BulletText("%d edges", (int)mesh_.n_edges());
        ImGui::BulletText("%d faces", (int)mesh_.n_faces());

        // control crease angle
        // TODO: Figure out if and how we need to use this. Seems useful?
        // ImGui::PushItemWidth(100);
        // ImGui::SliderFloat("Crease Angle", &crease_angle_, 0.0f, 180.0f, "%.0f");
        // ImGui::PopItemWidth();
        // if (crease_angle_ != renderer_.crease_angle())
        // {
        //     renderer_.set_crease_angle(crease_angle_);
        // }
    }
}

//! this function handles keyboard events
void CustomMeshViewer::keyboard(int key, int code, int action, int mod)
{
    if (action != GLFW_PRESS && action != GLFW_REPEAT)
        return;

    switch (key)
    {
        // case GLFW_KEY_BACKSPACE: // reload model
        // {
        //     if (!filename_.empty())
        //         load_mesh(filename_.c_str());
        //     break;
        // }

        // case GLFW_KEY_W: // write mesh
        // {
        //     write(mesh_, "output.off");
        //     break;
        // }

    default:
    {
        TrackballViewer::keyboard(key, code, action, mod);
        break;
    }
    }
}

//! get vertex closest to 3D position under the mouse cursor
pmp::Vertex CustomMeshViewer::pick_vertex(int x, int y)
{
    pmp::Vertex vmin;
    pmp::vec3 p;
    pmp::Scalar d, dmin(std::numeric_limits<pmp::Scalar>::max());

    if (TrackballViewer::pick(x, y, p))
    {
        pmp::Point picked_position(p);
        for (auto v : mesh_.vertices())
        {
            d = distance(mesh_.position(v), picked_position);
            if (d < dmin)
            {
                dmin = d;
                vmin = v;
            }
        }
    }
    return vmin;
}

void CustomMeshViewer::notify_shader_reload_required(ShaderType shader_typ)
{
    shader_reload_required_map_[shader_typ] = true;
}

} // namespace meshlife
