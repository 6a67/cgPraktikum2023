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
#include "meshlife/visualization/viewer.h"
#include "pmp/algorithms/differential_geometry.h"
#include "pmp/io/io.h"
#include "pmp/mat_vec.h"
#include "pmp/surface_mesh.h"
#include "pmp/types.h"

namespace meshlife
{

Viewer::Viewer(const char* title, int width, int height) : pmp::MeshViewer(title, width, height)
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

    modelpath_buf = new char[300];
    for (int i = 0; i < 300; i++)
    {
        modelpath_buf[i] = 0;
    }

    std::string("./../assets/monkey.obj").copy(modelpath_buf, 299);
    modelpath_buf[299] = '\0';

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

    peak_string = new char[300];

    add_help_item("T", "Toggle Simulation");
    add_help_item("R", "Load random state");
    add_help_item("S", "Step once in simulation");
    add_help_item("D", "Select face and retrieve debug info");
    add_help_item("O", "Reload custom shader from file");

    clock_last = std::chrono::high_resolution_clock::now();

    selected_shader_path_vertex_ = shaders_path_ / PATH_CUSTOM_SHADER_VERTEX;
    selected_shader_path_fragment_ = shaders_path_ / PATH_CUSTOM_SHADER_FRAGMENT;

    last_modified_shader_file_vertex_ = std::filesystem::last_write_time(selected_shader_path_vertex_);
    last_modified_shader_file_fragment_ = std::filesystem::last_write_time(selected_shader_path_fragment_);

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
    delete modelpath_buf;
}

void Viewer::on_close_callback()
{
    stop_simulation();
    file_watcher_disable();
}

void Viewer::file_watcher_func()
{
    while (watch_shader_file_)
    {
        std::filesystem::file_time_type current_shader_file_vertex_last_modified
            = std::filesystem::last_write_time(selected_shader_path_vertex_);
        std::filesystem::file_time_type current_shader_file_fragment_last_modified
            = std::filesystem::last_write_time(selected_shader_path_fragment_);

        if (current_shader_file_fragment_last_modified > last_modified_shader_file_fragment_
            || current_shader_file_vertex_last_modified > last_modified_shader_file_vertex_)
        {
            last_modified_shader_file_vertex_ = current_shader_file_vertex_last_modified;
            last_modified_shader_file_fragment_ = current_shader_file_fragment_last_modified;

            reload_shader();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void Viewer::file_watcher_enable()
{
    watch_shader_file_ = true;
    file_watcher_thread = std::thread(&Viewer::file_watcher_func, this);
}

void Viewer::file_watcher_disable()
{
    watch_shader_file_ = false;
    file_watcher_thread.join();
}

void Viewer::start_simulation(bool single_step)
{
    if (simulation_running)
    {
        return;
    }
    simulation_running = true;

    if (single_step)
    {
        automaton->update_state(1);
        simulation_running = false;
    }
    else
        simulation_thread = std::thread(&Viewer::simulation_thread_func, this);
}

void Viewer::simulation_thread_func()
{
    while (simulation_running)
    {
        auto c_now = std::chrono::high_resolution_clock::now();
        auto delta_clock_cycles = c_now - clock_last;
        auto delta_ms = (delta_clock_cycles / CLOCKS_PER_SEC).count();
        // only start updating the state after it has been rendered, otherwise we start rendering and redraw mid frame
        if (!ready_for_display && (unlimited_limit_UPS_ || (delta_ms >= (1000.0 / (double)UPS_))))
        {
            // std::cout << "Update state" << std::endl;
            clock_last = std::chrono::high_resolution_clock::now();
            automaton->update_state(1);
            current_UPS_ = 1000.0 / delta_ms;
            ready_for_display = true;
        }
    }
}

void Viewer::stop_simulation()
{
    simulation_running = false;
    if (simulation_thread.joinable())
        simulation_thread.join();
}

void Viewer::reload_shader()
{
    renderer_.reload_shaders(selected_shader_path_vertex_, selected_shader_path_fragment_);
    renderer_.set_texture_shader_files(shaders_path_ / "passthrough.vert", shaders_path_ / "texture.frag");
}

void Viewer::set_mesh_properties()
{
    if (!mesh_.has_face_property("f:color"))
    {
        mesh_.add_face_property("f:color", pmp::Color{1, 1, 1});
        renderer_.update_opengl_buffers();
    }
    if (automaton)
        automaton->allocate_needed_properties();
}

void Viewer::retrieve_debug_info_for_selected_face()
{
    double x, y;
    cursor_pos(x, y);
    pmp::Face face;
    find_face(x, y, face);
    if (face.is_valid())
    {
        debug_data.selected_face_idx = face.idx();
        select_debug_info_face(debug_data.selected_face_idx);
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
        ready_for_display = true;
        break;
    case GLFW_KEY_S:
        start_simulation(true);
        ready_for_display = true;
        break;
    case GLFW_KEY_R:
        automaton->init_state_random();
        ready_for_display = true;
        break;
    case GLFW_KEY_T:
        if (simulation_running)
            stop_simulation();
        else
            start_simulation();
        break;
    case GLFW_KEY_O:
        reload_shader();
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
        set_draw_mode("Hidden Line");
        set_mesh_properties();
        update_mesh();

        break;
    }
    default:
    {
        MeshViewer::keyboard(key, scancode, action, mods);
        renderer_.keyboard(key, action);
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
    // ready_for_display gets set to true every time the simulation thread finishes one update, so we limit redraw calls
    // to be in sync with the simulation delay. uncomplete_updates is used to circumvent this sync behaviour and allows
    // to redraw the state[] array while it still gets updated in the simulation thread
    if (uncomplete_updates || ready_for_display)
    {
        // reset update flag
        ready_for_display = false;

        // calculate raninbow colors for each face
        if (automaton)
        {
            for (auto f : mesh_.faces())
            {
                auto state = automaton->state(f);
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
    if (debug_data.face.is_valid())
        automaton->set_state(debug_data.face, 0);

    if (selected_face.is_valid())
    {
        // highlight selected face
        debug_data.face = selected_face;
        automaton->set_state(selected_face, 1);
    }
    else
    {
        debug_data.face = pmp::Face();
    }
}

void Viewer::process_imgui()
{

    // Show mesh info in GUI via parent class
    pmp::MeshViewer::process_imgui();

    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::Text("Current UPS: %.0f", current_UPS_);
    {
        std::stringstream label;
        label << "Unrestricted render updates (allows visual  update mid-calculation): "
              << (uncomplete_updates ? "ON" : "OFF") << ")";
        if (ImGui::Button(label.str().c_str()))
        {
            uncomplete_updates = !uncomplete_updates;
        }
    }
    {

        std::stringstream limit_ups_text;
        limit_ups_text << "Unlimited UPS (Current: " << (unlimited_limit_UPS_ ? "ON" : "OFF") << ")";
        if (ImGui::Button(limit_ups_text.str().c_str()))
        {
            unlimited_limit_UPS_ = !unlimited_limit_UPS_;
        }

        ImGui::SliderInt("UPS", &UPS_, 1, 1000);
    }
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Debug Info (Press D on a face)"))
    {
        if (debug_data.face.is_valid())
        {
            ImGui::Text("Face Idx: %d", debug_data.face.idx());

            {
                auto halfedges = mesh_.halfedges(debug_data.face);
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
                for (pmp::Halfedge halfedge : mesh_.halfedges(debug_data.face))
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
                int vertex_count = mesh_.valence(debug_data.face);
                std::stringstream label;
                label << "Vertices of face: " << vertex_count;
                ImGui::Text("%s", label.str().c_str());
                ImGui::BeginListBox("##vertices", ImVec2(400, vertex_count * 18.5));
                for (pmp::Vertex vertex : mesh_.vertices(debug_data.face))
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
        ImGui::InputInt("Face Idx", &debug_data.selected_face_idx, 0);
        if (ImGui::Button("Previous face"))
        {
            debug_data.selected_face_idx -= 1;
            if (debug_data.selected_face_idx < 0)
                debug_data.selected_face_idx = mesh_.faces_size() - 1;
            select_debug_info_face(debug_data.selected_face_idx);
        }
        ImGui::SameLine();
        if (ImGui::Button("Select face"))
        {
            select_debug_info_face(debug_data.selected_face_idx);
        }
        ImGui::SameLine();
        if (ImGui::Button("Next face"))
        {
            debug_data.selected_face_idx += 1;
            if ((size_t)debug_data.selected_face_idx >= mesh_.faces_size())
                debug_data.selected_face_idx = 0;

            select_debug_info_face(debug_data.selected_face_idx);
        }
    }
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
            automaton->allocate_needed_properties();
            update_mesh();
        }

        if (ImGui::Button("Quad-Tri Subdivision"))
        {
            quad_tri_subdivision(mesh_);
            automaton->allocate_needed_properties();
            update_mesh();
        }

        if (ImGui::Button("Catmull-Clark Subdivision"))
        {
            catmull_clark_subdivision(mesh_);
            automaton->allocate_needed_properties();
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
            label << "Toggle Simulation (" << (simulation_running ? "RUNNING" : "OFFLINE") << ")";
            if (ImGui::Button(label.str().c_str()))
            {
                if (simulation_running)
                    stop_simulation();
                else
                    start_simulation();
            }
        }

        ImGui::Text("Gome of Life Step");
        ImGui::SameLine();
        if (ImGui::Button("Next"))
        {
            automaton->update_state(1);
        }

        ImGui::Text("Gome of Life Random");
        ImGui::SameLine();
        if (ImGui::Button("Random"))
        {
            automaton->init_state_random();
        }

        // threshold slider
        ImGui::Text("Thresholds");
        ImGui::PushItemWidth(100);
        ImGui::SliderInt("Upper", &automaton->p_upper_threshold_, automaton->p_lower_threshold_, 10);
        ImGui::PopItemWidth();
        ImGui::PushItemWidth(100);
        ImGui::SliderInt("Lower", &automaton->p_lower_threshold_, 1, 10);
        ImGui::PopItemWidth();

        if (automaton->p_upper_threshold_ < automaton->p_lower_threshold_)
        {
            automaton->p_upper_threshold_ = automaton->p_lower_threshold_;
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
    }

    ImGui::InputText("Path: ", modelpath_buf, 300);

    if (ImGui::Button("Load model from path"))
    {
        read_mesh_from_file(std::string(modelpath_buf));
    }

    if (auto* lenia = dynamic_cast<MeshLenia*>(automaton))
    {
        ImGui::SliderFloat("Mu", &lenia->p_mu, 0, 1);
        ImGui::SliderFloat("Sigma", &lenia->p_sigma, 0, 1);
        ImGui::SliderInt("T", &lenia->p_T, 1, 50);

        // TODO: recalculate neighbors
        float neighborhood_radius = lenia->p_neighborhood_radius / lenia->average_edge_length;
        ImGui::SliderFloat("Neighborhood Radius", &neighborhood_radius, 0, 20);
        lenia->p_neighborhood_radius = neighborhood_radius * lenia->average_edge_length;
        if (ImGui::Button("Recalculate Neighborhood"))
        {
            // TODO: move a_gol to a better place
            stop_simulation();
            lenia->allocate_needed_properties();
        }
        ImGui::LabelText("Avg. Neighbor count:", "%d", lenia->neighbor_count_avg);

        if (ImGui::Button("Visualize Kernel Shell"))
        {
            lenia->visualize_kernel_shell();
        }

        if (ImGui::Button("Visualize Kernel Skeleton"))
        {
            lenia->visualize_kernel_skeleton();
        }

        ImGui::InputText("Peaks: ", peak_string, 300);
        if (ImGui::Button("Update Peaks"))
        {
            std::cout << peak_string << std::endl;
            lenia->p_beta_peaks.clear();
            std::string s(peak_string);
            std::stringstream ss(s);
            std::string item;
            while (std::getline(ss, item, ','))
            {
                lenia->p_beta_peaks.push_back(std::stof(item));
                std::cout << std::stof(item) << std::endl;
            }

            std::string s2;
            for (auto peak : lenia->p_beta_peaks)
            {
                s2 += std::to_string(peak) + ",";
            }
            strcpy(peak_string, s2.c_str());
        }

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
            if (debug_data.face.is_valid())
                f = debug_data.face;
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
            ready_for_display = true;
        }

        if (ImGui::Button("Check if normalised"))
        {
            std::cout << lenia->norm_check() << std::endl;
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
                lenia->p_mu = 0.15;
                lenia->p_sigma = 0.017;
                lenia->p_beta_peaks = {1};
                lenia->p_T = 10;
                lenia->p_neighborhood_radius = 13 * lenia->average_edge_length;
                break;
            case 1:
                lenia->p_mu = 0.26;
                lenia->p_sigma = 0.036;
                lenia->p_beta_peaks = {0.5, 1, 0.667};
                lenia->p_T = 10;
                lenia->p_neighborhood_radius = 18 * lenia->average_edge_length;
                break;
            }

            lenia->allocate_needed_properties();
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

    ImGui::Text("IMGui FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Calculated FPS: %.0f", renderer_.framerate);
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
            automaton->set_state(face, 1.0);
            if (auto* lenia = dynamic_cast<MeshLenia*>(automaton))
            {
                lenia->highlight_neighbors(face);
                std::cout << "highlighted neighbors" << std::endl;
            }
        }
    }
    else
    {
        MeshViewer::mouse(button, action, mods);
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
