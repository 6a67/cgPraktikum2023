#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <format>
#include <pmp/algorithms/decimation.h>
#include <pmp/algorithms/remeshing.h>
#include <pmp/algorithms/shapes.h>
#include <pmp/algorithms/subdivision.h>
#include <pmp/algorithms/utilities.h>

#include <imgui.h>
#include <sstream>

#include "meshlife/algorithms/mesh_lenia.h"
#include "meshlife/stamps.h"
#include "meshlife/visualization/custom_renderer.h"
#include "meshlife/visualization/viewer.h"
#include "pmp/algorithms/differential_geometry.h"
#include "pmp/io/io.h"
#include "pmp/mat_vec.h"
#include "pmp/surface_mesh.h"
#include "pmp/types.h"

namespace meshlife
{

Viewer::Viewer(const char* title, int width, int height) : CustomMeshViewer(title, width, height)
{
    // Load planar grid mesh by default
    mesh_.assign(pmp::quad_sphere(3));
    pmp::BoundingBox bb = bounds(mesh_);
    set_scene((pmp::vec3)bb.center(), 0.5 * bb.size());
    set_draw_mode("Hidden Line");
    set_draw_mode("Skybox only");
    set_draw_mode("Custom Shader");

    set_mesh_properties();

    update_mesh();

    modelpath_buf_ = new char[300];
    for (int i = 0; i < 300; i++)
    {
        modelpath_buf_[i] = 0;
    }

    std::string("./../assets/monkey.obj").copy(modelpath_buf_, 299);
    modelpath_buf_[299] = '\0';

    shaders_path_ = std::filesystem::current_path() / "shaders";
    if (!std::filesystem::exists(shaders_path_))
    {
        std::cout << "Could not find shaders folder in current directory! Using ../src/shaders" << std::endl;
        shaders_path_ = std::filesystem::current_path() / ".." / "src" / "shaders";
    }
    else
    {
        std::cout << "Found shaders folder in current directory! Using ./shaders" << std::endl;
    }

    peak_string_ = new char[300];

    add_help_item("T", "Toggle Simulation");
    add_help_item("R", "Load random state");
    add_help_item("S", "Step once in simulation");
    add_help_item("D", "Select face and retrieve debug info");
    add_help_item("O", "Reload custom shader from file");

    clock_last_ = std::chrono::high_resolution_clock::now();

    selected_shader_path_vertex_ = shaders_path_ / PATH_SIMPLE_SHADER_VERTEX_;
    selected_shader_path_fragment_ = shaders_path_ / PATH_SIMPLE_SHADER_FRAGMENT_;

    for (size_t i = 0; i < (size_t)ShaderType::COUNT; i++)
        last_modified_shader_files_.push_back(
            std::make_pair(std::filesystem::last_write_time(get_path_from_shader_type((ShaderType)i)), (ShaderType)i));

    reload_shader();

    // TODO: Disable for release
    file_watcher_enable();

    // list all files in src/shaders folder

    for (const auto& entry : std::filesystem::directory_iterator(shaders_path_))
    {
        std::string path = entry.path().string();
        if (path.find(".frag") != std::string::npos)
        {
            std::cout << "Found fragment shader: " << path << std::endl;
            shader_files_fragment_.push_back(path);
        }
    }
}

Viewer::~Viewer()
{
    delete modelpath_buf_;
}

void Viewer::on_close_callback()
{
    stop_simulation();
    file_watcher_disable();
}

std::filesystem::path Viewer::get_path_from_shader_type(ShaderType shader_type)
{
    switch (shader_type)
    {
    case ShaderType::SimpleVert:
        return shaders_path_ / selected_shader_path_vertex_;
    case ShaderType::SimpleFrag:
        return shaders_path_ / selected_shader_path_fragment_;

    case ShaderType::SkyboxVert:
        return shaders_path_ / "skybox.vert";
    case ShaderType::SkyboxFrag:
        return shaders_path_ / "skybox.frag";

    case ShaderType::ReflectiveSphereVert:
        return shaders_path_ / "reflective_sphere.vert";
    case ShaderType::ReflectiveSphereFrag:
        return shaders_path_ / "reflective_sphere.frag";

    case ShaderType::PhongVert:
        return shaders_path_ / "phong.vert";
    case ShaderType::PhongFrag:
        return shaders_path_ / "phong.frag";

    case ShaderType::COUNT:
        throw std::runtime_error("Invalid shader type");
    }
    throw std::runtime_error("Invalid shader type");
}

void Viewer::file_watcher_func()
{
    while (watch_shader_file_)
    {

        for (auto& [last_modified, shader_type] : last_modified_shader_files_)
        {
            std::filesystem::file_time_type current_last_modified
                = std::filesystem::last_write_time(get_path_from_shader_type(shader_type));

            if (current_last_modified > last_modified)
            {
                last_modified = current_last_modified;
                std::cout << "Reloading shader " << get_path_from_shader_type(shader_type) << std::endl;

                // TODO: Replace this with a single function load_shader(shader_type)
                {
                    switch (shader_type)
                    {
                    case ShaderType::SimpleVert:
                        notify_shader_reload_required(ShaderType::SimpleVert);
                        break;
                    case ShaderType::SimpleFrag:
                        notify_shader_reload_required(ShaderType::SimpleFrag);
                        break;

                    case ShaderType::SkyboxVert:
                        notify_shader_reload_required(ShaderType::SkyboxVert);
                        break;
                    case ShaderType::SkyboxFrag:
                        notify_shader_reload_required(ShaderType::SkyboxFrag);
                        break;

                    case ShaderType::ReflectiveSphereVert:
                        notify_shader_reload_required(ShaderType::ReflectiveSphereVert);
                        break;
                    case ShaderType::ReflectiveSphereFrag:
                        notify_shader_reload_required(ShaderType::ReflectiveSphereFrag);
                        break;

                    case ShaderType::PhongVert:
                        notify_shader_reload_required(ShaderType::PhongVert);
                        break;
                    case ShaderType::PhongFrag:
                        notify_shader_reload_required(ShaderType::PhongFrag);
                        break;

                    case ShaderType::COUNT:
                        throw std::runtime_error("Invalid shader type");
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
            }
        }
    }
}

void Viewer::file_watcher_enable()
{
    watch_shader_file_ = true;
    file_watcher_thread_ = std::thread(&Viewer::file_watcher_func, this);
}

void Viewer::file_watcher_disable()
{
    watch_shader_file_ = false;
    file_watcher_thread_.join();
}

void Viewer::start_simulation(bool single_step)
{
    if (simulation_running_)
    {
        return;
    }
    simulation_running_ = true;

    if (single_step)
    {
        automaton_->update_state(1);
        simulation_running_ = false;
    }
    else
        simulation_thread_ = std::thread(&Viewer::simulation_thread_func, this);
}

void Viewer::simulation_thread_func()
{
    while (simulation_running_)
    {
        auto c_now = std::chrono::high_resolution_clock::now();
        auto delta_clock_cycles = c_now - clock_last_;
        auto delta_ms = (delta_clock_cycles / CLOCKS_PER_SEC).count();
        // only start updating the state after it has been rendered, otherwise we start rendering and redraw mid
        // frame
        if (!ready_for_display_ && (unlimited_limit_UPS_ || (delta_ms >= (1000.0 / (double)UPS_))))
        {
            // std::cout << "Update state" << std::endl;
            clock_last_ = std::chrono::high_resolution_clock::now();
            automaton_->update_state(1);
            current_UPS_ = 1000.0 / delta_ms;
            ready_for_display_ = true;
        }
    }
}

void Viewer::stop_simulation()
{
    simulation_running_ = false;
    if (simulation_thread_.joinable())
        simulation_thread_.join();
}

void Viewer::reload_shader()
{
    renderer_.set_simple_shader_files(get_path_from_shader_type(ShaderType::SimpleVert),
                                      get_path_from_shader_type(ShaderType::SimpleFrag));
    renderer_.set_skybox_shader_files(get_path_from_shader_type(ShaderType::SkyboxVert),
                                      get_path_from_shader_type(ShaderType::SkyboxFrag));
    renderer_.set_reflective_sphere_shader_files(get_path_from_shader_type(ShaderType::ReflectiveSphereVert),
                                                 get_path_from_shader_type(ShaderType::ReflectiveSphereFrag));
    renderer_.set_phong_shader_files(get_path_from_shader_type(ShaderType::PhongVert),
                                     get_path_from_shader_type(ShaderType::PhongFrag));
}

void Viewer::set_mesh_properties()
{
    if (!mesh_.has_face_property("f:color"))
    {
        mesh_.add_face_property("f:color", pmp::Color{1, 1, 1});
        renderer_.update_opengl_buffers();
    }
    if (automaton_)
        automaton_->allocate_needed_properties();
}

void Viewer::retrieve_debug_info_for_selected_face()
{
    double x, y;
    cursor_pos(x, y);
    pmp::Face face;
    find_face(x, y, face);
    if (face.is_valid())
    {
        debug_data_.selected_face_idx_ = face.idx();
        select_debug_info_face(debug_data_.selected_face_idx_);
    }
}

void Viewer::keyboard(int key, int scancode, int action, int mods)
{
    // only process press and repeat action (no release or other)
    if (action != GLFW_PRESS && action != GLFW_REPEAT)
        return;

    switch (key)
    {
    case GLFW_KEY_D:
        retrieve_debug_info_for_selected_face();
        ready_for_display_ = true;
        break;
    case GLFW_KEY_S:
        start_simulation(true);
        ready_for_display_ = true;
        break;
    case GLFW_KEY_R:
        automaton_->init_state_random();
        ready_for_display_ = true;
        break;
    case GLFW_KEY_T:
        if (simulation_running_)
            stop_simulation();
        else
            start_simulation();
        break;
    case GLFW_KEY_O:
        reload_shader();
        break;

    case GLFW_KEY_C:
        renderer_.set_cam_direction(
            (CamDirection)(((int)renderer_.get_cam_direction() + 1) % (int)CamDirection::COUNT));
        std::cout << (int)renderer_.get_cam_direction() << " - "
                  << renderer_.direction_names_[(int)renderer_.get_cam_direction()];
        break;
    // num keys: load simple primitive meshes
    case GLFW_KEY_0:
    case GLFW_KEY_1:
    case GLFW_KEY_2:
    case GLFW_KEY_3:
    case GLFW_KEY_4:
    case GLFW_KEY_5:
    case GLFW_KEY_6:
    case GLFW_KEY_7:
    case GLFW_KEY_8:
    case GLFW_KEY_9:
    {
        // deselect face
        select_debug_info_face(-1);
        // stop simulation thread
        stop_simulation();

        switch (key)
        {
        case GLFW_KEY_0:
            mesh_.assign(pmp::plane());
            break;
        case GLFW_KEY_1:
            mesh_.assign(pmp::tetrahedron());
            break;
        case GLFW_KEY_2:
            mesh_.assign(pmp::octahedron());
            break;
        case GLFW_KEY_3:
            mesh_.assign(pmp::hexahedron());
            break;
        case GLFW_KEY_4:
            mesh_.assign(pmp::icosahedron());
            break;
        case GLFW_KEY_5:
            mesh_.assign(pmp::dodecahedron());
            break;
        case GLFW_KEY_6:
            mesh_.assign(pmp::icosphere(3));
            break;
        case GLFW_KEY_7:
            mesh_.assign(pmp::quad_sphere(3));
            break;
        case GLFW_KEY_8:
            mesh_.assign(pmp::uv_sphere());
            break;
        case GLFW_KEY_9:
            mesh_.assign(pmp::torus());
            break;
        }

        pmp::BoundingBox bb = bounds(mesh_);
        set_scene((pmp::vec3)bb.center(), 0.5 * bb.size());
        // set_draw_mode("Hidden Line");
        set_mesh_properties();
        update_mesh();

        break;
    }
    default:
    {
        CustomMeshViewer::keyboard(key, scancode, action, mods);
        break;
    }
    }

    if (!mesh_.has_face_property("f:color"))
    {
        mesh_.add_face_property("f:color", pmp::Color{1, 1, 1});
        renderer_.update_opengl_buffers();
    }
}

void Viewer::do_processing()
{

    // do_processing gets called every draw frame (most likely 60fps) so this limits the update rate
    // ready_for_display gets set to true every time the simulation thread finishes one update, so we limit
    // redraw calls to be in sync with the simulation delay. uncomplete_updates is used to circumvent this sync
    // behaviour and allows to redraw the state[] array while it still gets updated in the simulation thread
    if (uncomplete_updates_ || ready_for_display_)
    {
        // reset update flag
        ready_for_display_ = false;

        // calculate raninbow colors for each face
        if (automaton_)
        {
            for (auto f : mesh_.faces())
            {
                auto state = automaton_->state(f);
                float v = std::clamp(state, 0.0f, 1.0f);
                // make hsv rainbow color
                set_face_color(f, hsv_to_rgb((int)(v * 360 + 270) % 360, state, 1));
            }
        }
        // redraw new state
        renderer_.update_opengl_buffers();
    }

    // TODO: DELETE THIS? CHANGE REDRAW BEHAVIOUR TO UPDATE FOR SHADERS
    //  redraw new state
    renderer_.update_opengl_buffers();
}

pmp::Color Viewer::hsv_to_rgb(float h, float s, float v)
{
    float c = v * s;
    float x = c * (1 - std::abs(std::fmod(h / 60.0, 2) - 1));
    float m = v - c;

    float r, g, b;
    if (h >= 0 && h < 60)
    {
        r = c;
        g = x;
        b = 0;
    }
    else if (h >= 60 && h < 120)
    {
        r = x;
        g = c;
        b = 0;
    }
    else if (h >= 120 && h < 180)
    {
        r = 0;
        g = c;
        b = x;
    }
    else if (h >= 180 && h < 240)
    {
        r = 0;
        g = x;
        b = c;
    }
    else if (h >= 240 && h < 300)
    {
        r = x;
        g = 0;
        b = c;
    }
    else
    {
        r = c;
        g = 0;
        b = x;
    }

    return pmp::Color{r + m, g + m, b + m};
}

void Viewer::read_mesh_from_file(std::string path)
{
    std::cout << "Loading mesh from: " << path << std::endl;
    std::filesystem::path file{path};
    if (std::filesystem::exists(file))
    {
        pmp::read(mesh_, file);
        set_mesh_properties();
        update_mesh();
    }
    else
    {
        std::cerr << "Error: Filepath does not exist!" << std::endl;
    }

    // mesh_.assign(mesh);
}

void Viewer::select_debug_info_face(size_t face_idx)
{
    pmp::Face selected_face = pmp::Face();
    for (auto face : mesh_.faces())
    {
        if (face.idx() == face_idx)
        {
            selected_face = face;
            break;
        }
    }

    // reset previous selected face
    if (debug_data_.face_.is_valid())
        automaton_->set_state(debug_data_.face_, 0);

    if (selected_face.is_valid())
    {
        // highlight selected face
        debug_data_.face_ = selected_face;
        automaton_->set_state(selected_face, 1);
    }
    else
    {
        debug_data_.face_ = pmp::Face();
    }
}

// Call this right after creating the object e.g. after using ImGui::Button(...);
#define IMGUI_TOOLTIP_TEXT(TEXT)                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))                                                 \
        {                                                                                                              \
            ImGui::SetTooltip(TEXT);                                                                                   \
        }                                                                                                              \
    } while (0);

void Viewer::process_imgui()
{
    ImGui::Text("IMGui FPS: %.1f", ImGui::GetIO().Framerate == FLT_MAX ? 0 : ImGui::GetIO().Framerate);
    IMGUI_TOOLTIP_TEXT("This FPS is averaged over last 60 frames")
    ImGui::Text("Calculated FPS: %.0f", renderer_.get_framerate());
    IMGUI_TOOLTIP_TEXT("This FPS is calculated with the time between the last two draw calls")
    ImGui::Separator();
    ImGui::Text("Current UPS (Simulation): %.0f", current_UPS_);
    IMGUI_TOOLTIP_TEXT("This FPS is calculated with the time between the last two draw calls")
    ImGui::Separator();
    ImGui::Text("iTime (Shader): %.2f", renderer_.get_itime());
    ImGui::Text("Current Draw Mode: %s", draw_mode_names_[draw_mode_].c_str());
    ImGui::Text("Last Cam-Direction: %s", renderer_.direction_names_[(int)renderer_.get_cam_direction()].c_str());
    ImGui::Text("Selected Shader: %s", get_path_from_shader_type(ShaderType::SimpleFrag).filename().c_str());
    ImGui::Separator();

    // Show mesh info in GUI via parent class
    CustomMeshViewer::process_imgui();

    if (ImGui::CollapsingHeader("Mesh Transform Settings"))
    {
        {
            std::stringstream position_text;
            position_text << center_;
            ImGui::Text("Mesh Center Position: %s", position_text.str().c_str());
            IMGUI_TOOLTIP_TEXT("The center point of the mesh (in local space)");
        }
        {
            std::stringstream position_text;
            pmp::vec3 model_pos = position_;
            position_text << model_pos;
            ImGui::Text("Mesh Position: %s", position_text.str().c_str());
            IMGUI_TOOLTIP_TEXT("Mesh world position");
        }
        {
            std::stringstream modelview_matrix_text;
            modelview_matrix_text << get_modelview_matrix();
            ImGui::Text("Modelview matrix:\n%s", modelview_matrix_text.str().c_str());
        }

        ImGui::Separator();

        if (ImGui::Button("Reset Mesh to origin"))
        {
            position_ = pmp::vec3(0.0f);
        }

        ImGui::SameLine();

        if (ImGui::Button("Reset Mesh Orientation"))
        {
            rotation_matrix_ = pmp::mat4::identity();
        }

        ImGui::SameLine();

        if (ImGui::Button("Reset Mesh scale"))
        {
            mesh_size_uniform_ = 1.0;
            mesh_size_x_ = 1.0;
            mesh_size_y_ = 1.0;
            mesh_size_z_ = 1.0;
            set_mesh_scale(1.0);
            update_mesh();
        }

        if (ImGui::SliderFloat("Mesh scale Uniformly:", &mesh_size_uniform_, 0.01, 40))
        {
            pmp::vec3 scaling = pmp::vec3(mesh_size_uniform_, mesh_size_uniform_, mesh_size_uniform_);
            mesh_size_x_ = mesh_size_uniform_;
            mesh_size_y_ = mesh_size_uniform_;
            mesh_size_z_ = mesh_size_uniform_;
            set_mesh_scale(scaling);
            update_mesh();
        }

        if (ImGui::SliderFloat("Mesh scale X:", &mesh_size_x_, 0.01, 40))
        {
            pmp::vec3 scaling = pmp::vec3(mesh_size_x_, mesh_size_y_, mesh_size_z_);
            set_mesh_scale(scaling);
            update_mesh();
        }
        if (ImGui::SliderFloat("Mesh scale Y:", &mesh_size_y_, 0.01, 40))
        {
            pmp::vec3 scaling = pmp::vec3(mesh_size_x_, mesh_size_y_, mesh_size_z_);
            set_mesh_scale(scaling);
            update_mesh();
        }
        if (ImGui::SliderFloat("Mesh scale Z:", &mesh_size_z_, 0.01, 40))
        {
            pmp::vec3 scaling = pmp::vec3(mesh_size_x_, mesh_size_y_, mesh_size_z_);
            set_mesh_scale(scaling);
            update_mesh();
        }
    }

    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Shader Settings"))
    {
        if (ImGui::Button("Step Backwards"))
        {
            renderer_.set_itime(std::max(0.0f, (float)renderer_.get_itime() - 0.1f));
        }

        ImGui::SameLine();

        if (ImGui::Button(renderer_.get_itime_paused() ? "Continue iTime" : "Pause iTime"))
        {
            renderer_.itime_toggle_pause();
        }

        ImGui::SameLine();

        if (ImGui::Button("Step Forwards"))
        {
            renderer_.set_itime(renderer_.get_itime() + 0.1);
        }

        if (ImGui::Button("Reset iTime"))
        {
            renderer_.set_itime();
        }

        if (ImGui::Button(renderer_.use_picture_cubemap_ ? "Swap cubemap: Picture" : "Swap cubemap: Shader cubemap"))
        {
            renderer_.use_picture_cubemap_ = !renderer_.use_picture_cubemap_;
        }
        IMGUI_TOOLTIP_TEXT(
            "(Only applies in 'Skybox' draw mode, swaps between rendered shader texture and debug texture)");

        ImGui::Separator();

        for (size_t i = 0; i < n_draw_modes_; i++)
        {
            std::stringstream label;
            label << "Draw Mode: " << draw_mode_names_[i];
            if (ImGui::Button(label.str().c_str()))
            {
                draw_mode_ = i;
            }
        }
        ImGui::Separator();

        size_t direction_max = (size_t)CamDirection::COUNT;
        for (size_t i = 0; i < direction_max; i++)
        {
            std::stringstream label;
            label << "Direction: " << renderer_.direction_names_[i];
            if (ImGui::Button(label.str().c_str()))
            {
                renderer_.set_cam_direction((CamDirection)(i));
            }
        }
        ImGui::Separator();
    }

    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("UPS Settings (for simulation)"))
    {
        {
            // std::stringstream label;
            // label << "Unrestricted render updates (allows visual  update mid-calculation): "
            //       << (uncomplete_updates_ ? "ON" : "OFF") << ")";
            // if (ImGui::Button(label.str().c_str()))
            // {
            //     uncomplete_updates_ = !uncomplete_updates_;
            // }
            // IMGUI_TOOLTIP_TEXT("(HAS NO EFFECT ANYMORE)");
        } {

            std::stringstream limit_ups_text;
            limit_ups_text << "Unlimited UPS (Current: " << (unlimited_limit_UPS_ ? "ON" : "OFF") << ")";
            if (ImGui::Button(limit_ups_text.str().c_str()))
            {
                unlimited_limit_UPS_ = !unlimited_limit_UPS_;
            }
            IMGUI_TOOLTIP_TEXT("Limits the updates per second of the lenia simulation");

            ImGui::SliderInt("UPS", &UPS_, 1, 1000);
            IMGUI_TOOLTIP_TEXT("Updates per second of the lenia simulation");
        }
    }

    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Debug Info (Press D on a face)"))
    {
        if (debug_data_.face_.is_valid())
        {
            ImGui::Text("Face Idx: %d", debug_data_.face_.idx());

            {
                auto halfedges = mesh_.halfedges(debug_data_.face_);
                int count = 0;
                // TODO: Figure out a better way to count the edges
                for (auto h : halfedges)
                {
                    // this ignores unused variable warning
                    std::ignore = h;
                    count++;
                }
                std::stringstream title;
                title << "Halfedges of face: " << count;

                ImGui::Text("%s", title.str().c_str());
                ImGui::BeginListBox("##halfedges", ImVec2(-FLT_MIN, count * 18.5));
                for (pmp::Halfedge halfedge : mesh_.halfedges(debug_data_.face_))
                {
                    ImGui::Text("Halfedge idx: %d (opp. HE: %d | Face: %d) (Start: %d | End: %d)",
                                halfedge.idx(),
                                mesh_.opposite_halfedge(halfedge).idx(),
                                mesh_.face(mesh_.opposite_halfedge(halfedge)).idx(),
                                mesh_.from_vertex(halfedge).idx(),
                                mesh_.to_vertex(halfedge).idx());
                }
                ImGui::EndListBox();
            }

            {
                int vertex_count = mesh_.valence(debug_data_.face_);
                std::stringstream label;
                label << "Vertices of face: " << vertex_count;
                ImGui::Text("%s", label.str().c_str());
                ImGui::BeginListBox("##vertices", ImVec2(400, vertex_count * 18.5));
                for (pmp::Vertex vertex : mesh_.vertices(debug_data_.face_))
                {
                    auto pos = mesh_.position(vertex);
                    ImGui::Text("Vertex idx: %d XYZ: (%.02f, %.02f, %.02f)", vertex.idx(), pos[0], pos[1], pos[2]);
                }
                ImGui::EndListBox();
            }
        }
        else
        {
            ImGui::Text("No face selected. Press 'D' on a Face to get more info.");
        }
        ImGui::InputInt("Face Idx", &debug_data_.selected_face_idx_, 0);
        if (ImGui::Button("Previous face"))
        {
            debug_data_.selected_face_idx_ -= 1;
            if (debug_data_.selected_face_idx_ < 0)
                debug_data_.selected_face_idx_ = mesh_.faces_size() - 1;
            select_debug_info_face(debug_data_.selected_face_idx_);
        }
        ImGui::SameLine();
        if (ImGui::Button("Select face"))
        {
            select_debug_info_face(debug_data_.selected_face_idx_);
        }
        ImGui::SameLine();
        if (ImGui::Button("Next face"))
        {
            debug_data_.selected_face_idx_ += 1;
            if ((size_t)debug_data_.selected_face_idx_ >= mesh_.faces_size())
                debug_data_.selected_face_idx_ = 0;

            select_debug_info_face(debug_data_.selected_face_idx_);
        }
    }

    ImGui::Spacing();
    ImGui::Spacing();

    // Some functionality from pmp::MeshProcessingViewer to edit mesh
    if (ImGui::CollapsingHeader("Decimation"))
    {
        static int target_percentage = 10;
        ImGui::PushItemWidth(100);
        ImGui::SliderInt("Percentage", &target_percentage, 1, 99);
        ImGui::PopItemWidth();

        static int normal_deviation = 135;
        ImGui::PushItemWidth(100);
        ImGui::SliderInt("Normal Deviation", &normal_deviation, 1, 135);
        ImGui::PopItemWidth();

        static int aspect_ratio = 10;
        ImGui::PushItemWidth(100);
        ImGui::SliderInt("Aspect Ratio", &aspect_ratio, 1, 10);
        ImGui::PopItemWidth();

        static int seam_angle_deviation = 1;
        ImGui::PushItemWidth(100);
        ImGui::SliderInt("Seam Angle Deviation", &seam_angle_deviation, 0, 15);
        ImGui::PopItemWidth();

        if (ImGui::Button("Decimate"))
        {
            try
            {
                auto nv = mesh_.n_vertices() * 0.01 * target_percentage;
                decimate(mesh_, nv, aspect_ratio, 0.0, 0.0, normal_deviation, 0.0, 0.01, seam_angle_deviation);
            }
            catch (const pmp::InvalidInputException& e)
            {
                std::cerr << e.what() << std::endl;
                return;
            }
            update_mesh();
        }
    }

    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Subdivision"))
    {
        if (ImGui::Button("Loop Subdivision"))
        {
            try
            {
                loop_subdivision(mesh_);
            }
            catch (const pmp::InvalidInputException& e)
            {
                std::cerr << e.what() << std::endl;
                return;
            }
            automaton_->allocate_needed_properties();
            update_mesh();
        }

        if (ImGui::Button("Quad-Tri Subdivision"))
        {
            quad_tri_subdivision(mesh_);
            automaton_->allocate_needed_properties();
            update_mesh();
        }

        if (ImGui::Button("Catmull-Clark Subdivision"))
        {
            catmull_clark_subdivision(mesh_);
            automaton_->allocate_needed_properties();
            update_mesh();
        }
    }

    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Remeshing"))
    {
        if (ImGui::Button("Adaptive Remeshing"))
        {
            auto bb = bounds(mesh_).size();

            try
            {
                adaptive_remeshing(mesh_,
                                   0.001 * bb,  // min length
                                   1.0 * bb,    // max length
                                   0.001 * bb); // approx. error
            }
            catch (const pmp::InvalidInputException& e)
            {
                std::cerr << e.what() << std::endl;
                return;
            }
            update_mesh();
        }

        if (ImGui::Button("Uniform Remeshing"))
        {
            pmp::Scalar l(0);
            for (auto eit : mesh_.edges())
                l += distance(mesh_.position(mesh_.vertex(eit, 0)), mesh_.position(mesh_.vertex(eit, 1)));
            l /= (pmp::Scalar)mesh_.n_edges();

            try
            {
                uniform_remeshing(mesh_, l);
            }
            catch (const pmp::InvalidInputException& e)
            {
                std::cerr << e.what() << std::endl;
                return;
            }
            update_mesh();
        }
    }

    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Algorithms"))
    {
        ImGui::Text("Game of Life Toggle");
        ImGui::SameLine();
        {
            std::stringstream label;
            label << "Toggle Simulation (" << (simulation_running_ ? "RUNNING" : "OFFLINE") << ")";
            if (ImGui::Button(label.str().c_str()))
            {
                if (simulation_running_)
                    stop_simulation();
                else
                    start_simulation();
            }
        }

        ImGui::Text("Gome of Life Step");
        ImGui::SameLine();
        if (ImGui::Button("Next"))
        {
            automaton_->update_state(1);
        }

        ImGui::Text("Gome of Life Random");
        ImGui::SameLine();
        if (ImGui::Button("Random"))
        {
            automaton_->init_state_random();
        }

        // threshold slider
        ImGui::Text("Thresholds");
        ImGui::PushItemWidth(100);
        ImGui::SliderInt("Upper", &automaton_->p_upper_threshold_, automaton_->p_lower_threshold_, 10);
        ImGui::PopItemWidth();
        ImGui::PushItemWidth(100);
        ImGui::SliderInt("Lower", &automaton_->p_lower_threshold_, 1, 10);
        ImGui::PopItemWidth();

        if (automaton_->p_upper_threshold_ < automaton_->p_lower_threshold_)
        {
            automaton_->p_upper_threshold_ = automaton_->p_lower_threshold_;
        }
    }

    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Dual Meshes"))
    {
        if (ImGui::Button("Create Dual Mesh"))
        {
            pmp::dual(mesh_);
            set_mesh_properties();
        }
        IMGUI_TOOLTIP_TEXT("Converts the current mesh to its dual mesh variant");
    }
    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Load custom model file"))
    {
        ImGui::InputText("Path: ", modelpath_buf_, 300);
        if (ImGui::Button("Load model from path"))
        {
            read_mesh_from_file(std::string(modelpath_buf_));
        }
        IMGUI_TOOLTIP_TEXT("Loads the model as current mesh. Drag&Drop'ing the mesh file is also supported.");
    }

    if (auto* lenia = dynamic_cast<MeshLenia*>(automaton_))
    {
        ImGui::Spacing();
        ImGui::Spacing();
        if (ImGui::CollapsingHeader("Lenia Parameters"))
        {
            ImGui::SliderFloat("Mu", &lenia->p_mu_, 0, 1);
            ImGui::SliderFloat("Sigma", &lenia->p_sigma_, 0, 1);
            ImGui::SliderInt("T", &lenia->p_T_, 1, 50);

            // TODO: recalculate neighbors
            float neighborhood_radius = lenia->p_neighborhood_radius_ / lenia->average_edge_length_;
            ImGui::SliderFloat("Neighborhood Radius", &neighborhood_radius, 0, 20);
            lenia->p_neighborhood_radius_ = neighborhood_radius * lenia->average_edge_length_;
            if (ImGui::Button("Recalculate Neighborhood"))
            {
                // TODO: move a_gol to a better place
                stop_simulation();
                lenia->allocate_needed_properties();
            }
            IMGUI_TOOLTIP_TEXT("Manually recalculates the neighboorhood map for the lenia simulation.");
            ImGui::LabelText("Avg. Neighbor count:", "%d", lenia->neighbor_count_avg_);

            if (ImGui::Button("Visualize Kernel Shell"))
            {
                lenia->visualize_kernel_shell();
            }

            if (ImGui::Button("Visualize Kernel Skeleton"))
            {
                lenia->visualize_kernel_skeleton();
            }

            ImGui::InputText("Peaks: ", peak_string_, 300);
            if (ImGui::Button("Update Peaks"))
            {
                std::cout << peak_string_ << std::endl;
                lenia->p_beta_peaks_.clear();
                std::string s(peak_string_);
                std::stringstream ss(s);
                std::string item;
                while (std::getline(ss, item, ','))
                {
                    lenia->p_beta_peaks_.push_back(std::stof(item));
                    std::cout << std::stof(item) << std::endl;
                }

                std::string s2;
                for (auto peak : lenia->p_beta_peaks_)
                {
                    s2 += std::to_string(peak) + ",";
                }
                strcpy(peak_string_, s2.c_str());
            }

            if (ImGui::CollapsingHeader("Stamps"))
            {
                static stamps::Shapes stamp;
                std::stringstream label;
                label << "Stamp: " << stamps::shape_to_str(stamp);
                if (ImGui::BeginMenu(label.str().c_str()))
                {
                    if (ImGui::MenuItem("Orbium"))
                        stamp = stamps::s_orbium;
                    else if (ImGui::MenuItem("Smiley"))
                        stamp = stamps::s_smiley;
                    else if (ImGui::MenuItem("Debug"))
                        stamp = stamps::s_debug;
                    else if (ImGui::MenuItem("Geminium"))
                        stamp = stamps::s_geminium;
                    ImGui::EndMenu();
                }

                if (ImGui::Button("Place Stamp (Select startface with 'D')"))
                {
                    pmp::Face f;
                    if (debug_data_.face_.is_valid())
                        f = debug_data_.face_;
                    switch (stamp)
                    {
                    case stamps::Shapes::s_none:
                        break;
                    case stamps::Shapes::s_orbium:
                        lenia->place_stamp(f, stamps::orbium);
                        break;
                    case stamps::Shapes::s_smiley:
                        lenia->place_stamp(f, stamps::smiley);
                        break;
                    case stamps::Shapes::s_debug:
                        lenia->place_stamp(f, stamps::debug);
                        break;
                    case stamps::Shapes::s_geminium:
                        lenia->place_stamp(f, stamps::geminium);
                        break;
                    }
                    // cause the render to redraw the next draw frame
                    ready_for_display_ = true;
                }
                IMGUI_TOOLTIP_TEXT("Select a stamp and a face (with D) and then press this button to place the stamp");
            }

            // list from where presets can be selected
            static const char* items[] = {"Glider Settings", "Geminium Settings"};
            static int item_current = 1;
            if (ImGui::BeginCombo("Presets", items[item_current]))
            {
                for (int n = 0; n < IM_ARRAYSIZE(items); n++)
                {
                    bool is_selected = (item_current == n);
                    if (ImGui::Selectable(items[n], is_selected))
                        item_current = n;
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            if (ImGui::Button("Load preset"))
            {
                switch (item_current)
                {
                case 0:
                    lenia->p_mu_ = 0.15;
                    lenia->p_sigma_ = 0.017;
                    lenia->p_beta_peaks_ = {1};
                    lenia->p_T_ = 10;
                    lenia->p_neighborhood_radius_ = 13 * lenia->average_edge_length_;
                    break;
                case 1:
                    lenia->p_mu_ = 0.26;
                    lenia->p_sigma_ = 0.036;
                    lenia->p_beta_peaks_ = {0.5, 1, 0.667};
                    lenia->p_T_ = 10;
                    lenia->p_neighborhood_radius_ = 18 * lenia->average_edge_length_;
                    break;
                }

                lenia->allocate_needed_properties();
            }
        }
    }

    ImGui::Spacing();
    ImGui::Spacing();

    // select shader from file
    if (ImGui::CollapsingHeader("Shaders"))
    {
        for (auto sh : shader_files_fragment_)
        {
            std::string name = sh.substr(sh.find_last_of("/\\") + 1);
            if (ImGui::Button(name.c_str()))
            {
                selected_shader_path_fragment_ = sh;
                reload_shader();
            }
        }
    }
}

void Viewer::set_face_color(pmp::Face& face, pmp::Color color)
{
    auto color_property = mesh_.get_face_property<pmp::Color>("f:color");
    // std::cout << colorProperty[face] << std::endl;
    color_property[face] = color;
}

void Viewer::mouse(int button, int action, int mods)
{
    if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_RIGHT && Window::ctrl_pressed())
    {
        double x, y;
        cursor_pos(x, y);
        pmp::Vertex v = pick_vertex(x, y);
        if (mesh_.is_valid(v))
        {
            // You can do something with the picked vertex here
            pmp::Face face;
            find_face(x, y, face);
            // TODO: maybe convert to method
            automaton_->set_state(face, 1.0);
            if (auto* lenia = dynamic_cast<MeshLenia*>(automaton_))
            {
                lenia->highlight_neighbors(face);
                std::cout << "highlighted neighbors" << std::endl;
            }
        }
    }
    else
    {
        CustomMeshViewer::mouse(button, action, mods);
    }
}

bool Viewer::find_face(int x, int y, pmp::Face& face)
{
    pmp::vec3 p;
    pmp::Face fmin;
    pmp::Scalar d, dmin(std::numeric_limits<pmp::Scalar>::max());

    if (TrackballViewer::pick(x, y, p))
    {
        pmp::Point picked_position(p);
        for (auto f : mesh_.faces())
        {
            // TODO: This will not always return the correct face
            d = distance(pmp::centroid(mesh_, f), picked_position);
            if (d < dmin)
            {
                dmin = d;
                fmin = f;
            }
        }
    }
    face = fmin;
    return true;
}
} // namespace meshlife
