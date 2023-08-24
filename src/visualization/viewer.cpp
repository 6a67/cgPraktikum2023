#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <format>
#include <limits>
#include <pmp/algorithms/decimation.h>
#include <pmp/algorithms/remeshing.h>
#include <pmp/algorithms/shapes.h>
#include <pmp/algorithms/subdivision.h>
#include <pmp/algorithms/utilities.h>

#include <imgui.h>
#include <sstream>
#include <stb_image_write.h>
#include <thread>

#include "meshlife/algorithms/mesh_lenia.h"
#include "meshlife/paths.h"
#include "meshlife/stamps.h"
#include "meshlife/visualization/custom_renderer.h"
#include "meshlife/visualization/viewer.h"
#include "pmp/algorithms/differential_geometry.h"
#include "pmp/io/io.h"
#include "pmp/mat_vec.h"
#include "pmp/surface_mesh.h"
#include "pmp/types.h"

#include "GLFW/glfw3.h"

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
    set_draw_mode("Reflective Sphere");

    set_mesh_properties();

    update_mesh();

    modelpath_buf_ = new char[300];
    for (int i = 0; i < 300; i++)
    {
        modelpath_buf_[i] = 0;
    }

    std::string("./../assets/monkey.obj").copy(modelpath_buf_, 299);
    modelpath_buf_[299] = '\0';

    peak_string_ = new char[300];

    add_help_item("T", "Toggle Simulation");
    add_help_item("R", "Load random state");
    add_help_item("S", "Step once in simulation");
    add_help_item("D", "Select face and retrieve debug info");
    add_help_item("O", "Reload custom shader from file");
    add_help_item("C", "Rotate camera (for Debugging)");
    add_help_item("B", "Place selected stamp on mesh");

    clock_last_ = std::chrono::high_resolution_clock::now();

    selected_shader_path_vertex_ = shaders_path / PATH_SIMPLE_SHADER_VERTEX_;
    selected_shader_path_fragment_ = shaders_path / PATH_SIMPLE_SHADER_FRAGMENT_;

    for (size_t i = 0; i < (size_t)ShaderType::COUNT; i++)
        last_modified_shader_files_.push_back(
            std::make_pair(std::filesystem::last_write_time(get_path_from_shader_type((ShaderType)i)), (ShaderType)i));

    reload_shader();

    // TODO: Disable for release
    file_watcher_enable();

    // list all files in src/shaders folder
    for (const auto& entry : std::filesystem::directory_iterator(shaders_path))
    {
        std::string path = entry.path().string();
        if (path.find(".frag") != std::string::npos)
        {
            // std::cout << "Found fragment shader: " << path << std::endl;
            shader_files_fragment_.push_back(path);
        }
    }

    // set default position for model
    position_ = pmp::vec3(0.0f, 0.0f, -0.4f);

    set_mesh_scale(pmp::vec3(0.05f, 0.05f, 0.05f));

    renderer_.set_reflectiveness(0.3f);

    recordings_path_ = std::filesystem::current_path() / "recordings";
}

Viewer::~Viewer()
{
    delete modelpath_buf_;
}

void Viewer::on_close_callback()
{
    stop_simulation();
    file_watcher_disable();
    if (thread_ffmpeg_.joinable())
    {
        thread_ffmpeg_.join();
    }
}

void Viewer::start_recording()
{
    size_t size = (size_t)3 * (size_t)width() * (size_t)height() * (size_t)recording_buffer_count_;
    try
    {
        recording_frame_data_.resize(size);
    }
    catch (std::bad_alloc& e)
    {
        std::cerr << "Error: Not enough memory for buffer\n" << e.what() << std::endl;
        return;
    }
    catch (std::length_error& e)
    {
        std::cerr << "Error: Not enough memory for buffer\n" << e.what() << std::endl;
        return;
    }

    recording_start_time_ = std::chrono::system_clock::now();
    recording_image_counter_ = 0;
    // pause itime because we will step manually for the recording and not rely on glfwGetTime()
    renderer_.itime_pause();
    renderer_.set_itime(0.0);
    // Disable vsync for recording since we step manually
    glfwSwapInterval(0);
    recording_ = true;
    std::cout << "Recording started" << std::endl;
}

void Viewer::stop_recording()
{
    recording_ = false;

    // Enable vsync again
    glfwSwapInterval(1);

    std::cout << "Recording stopped" << std::endl;

    std::cout << "Waiting for files to be written to disk..." << std::endl;
    join_recording_buffer_threads();
    std::cout << "Frames have been written to disk." << std::endl;

    if (recording_create_video_)
    {
        std::cout << "Converting to video..." << std::endl;
        recording_ffmpeg_is_converting_video_ = true;
        // make sure old thread is joined before replacing
        if (thread_ffmpeg_.joinable())
        {
            thread_ffmpeg_.join();
        }
        thread_ffmpeg_ = std::thread(&Viewer::convert_recorded_frames_to_video, this);
    }
    else
    {

        std::cout << recording_frame_target_count_ << " Frames saved to " << recordings_path_ << '\n'
                  << "Use ffmpeg to combine frames:\n"
                     "ffmpeg -framerate 60 -pattern_type glob -i 'frame_*"
                  << recording_fileformat_
                  << "' -c:v libx264 -pix_fmt yuv420p out.mp4\n"
                     "set framerate and pixel format accordingly (use 'ffmpeg -pix_fmts' for all possible formats)"
                  << std::endl;
    }
}

void Viewer::join_recording_buffer_threads()
{
    while (!recording_buffer_threads_.empty())
    {
        std::thread& thread = *recording_buffer_threads_.top();
        recording_buffer_threads_.pop();
        if (thread.joinable())
        {
            thread.join();
        }
    }
}

void Viewer::delete_recorded_frames()
{
    for (auto& filepath : std::filesystem::directory_iterator(recordings_path_))
    {
        std::string filename = filepath.path().filename().string();
        // Check if file starts with 'frame_' and ends with '.png'
        if (std::filesystem::is_regular_file(filepath) && filename.rfind("frame_") == 0
            && (filename.rfind(".png") == (filename.size() - 4) || filename.rfind(".jpg") == (filename.size() - 4)))
        {
            std::filesystem::remove(filepath);
        }
    }
}

void Viewer::convert_recorded_frames_to_video()
{
    std::stringstream command;
    command << "cd " << std::filesystem::absolute(recordings_path_) << " && "
            << "ffmpeg -framerate " << recording_framerate_ << " -y -pattern_type glob -i 'frame_*"
            << recording_fileformat_ << "' -c:v libx264 -pix_fmt yuv420p output.mp4\n";
    std::cout << "Running: '" << command.str() << "'" << std::endl;

    std::system(command.str().c_str());
    recording_ffmpeg_is_converting_video_ = false;
}

std::filesystem::path Viewer::get_path_from_shader_type(ShaderType shader_type)
{
    switch (shader_type)
    {
    case ShaderType::SimpleVert:
        return shaders_path / selected_shader_path_vertex_;
    case ShaderType::SimpleFrag:
        return shaders_path / selected_shader_path_fragment_;

    case ShaderType::SkyboxVert:
        return shaders_path / "skybox.vert";
    case ShaderType::SkyboxFrag:
        return shaders_path / "skybox.frag";

    case ShaderType::ReflectiveSphereVert:
        return shaders_path / "reflective_sphere.vert";
    case ShaderType::ReflectiveSphereFrag:
        return shaders_path / "reflective_sphere.frag";

    case ShaderType::PhongVert:
        return shaders_path / "phong.vert";
    case ShaderType::PhongFrag:
        return shaders_path / "phong.frag";

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

pmp::Face Viewer::get_face_under_cursor()
{
    double x, y;
    cursor_pos(x, y);
    pmp::Face face;
    find_face(x, y, face);
    if (face.is_valid())
    {
        return face;
    }
    return pmp::Face();
}

void Viewer::retrieve_debug_info_for_selected_face()
{
    pmp::Face face = get_face_under_cursor();
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
    case GLFW_KEY_B:
        if (auto* lenia = dynamic_cast<MeshLenia*>(automaton_))
        {
            pmp::Face f = get_face_under_cursor();
            if (!f.is_valid())
                break;

            switch (selected_stamp_)
            {
            case stamps::Shapes::s_none:
                break;
            case stamps::Shapes::s_orbium:
                lenia->place_stamp(f, stamps::orbium);
                ready_for_display_ = true;
                break;
            case stamps::Shapes::s_smiley:
                lenia->place_stamp(f, stamps::smiley);
                ready_for_display_ = true;
                break;
            case stamps::Shapes::s_debug:
                lenia->place_stamp(f, stamps::debug);
                ready_for_display_ = true;
                break;
            case stamps::Shapes::s_geminium:
                lenia->place_stamp(f, stamps::geminium);
                ready_for_display_ = true;
                break;
            case stamps::Shapes::s_gyrorbium:
                lenia->place_stamp(f, stamps::gyrorbium);
                ready_for_display_ = true;
                break;
            case stamps::Shapes::s_velox:
                lenia->place_stamp(f, stamps::velox);
                ready_for_display_ = true;
                break;
            case stamps::Shapes::s_circle:
                lenia->place_circle(f,
                                    stamp_circle_inner_ * lenia->average_edge_length_,
                                    stamp_circle_outer_ * lenia->average_edge_length_);
                ready_for_display_ = true;
                break;
            }
        }
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
            set_mesh_scale(pmp::vec3(0.05f, 0.05f, 0.05f));
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

        position_ = pmp::vec3(0.0f, 0.0f, -0.4f);

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

void Viewer::drop(int count, const char** paths)
{
    CustomMeshViewer::drop(count, paths);
    set_mesh_properties();
}

void Viewer::after_display()
{
    if (recording_)
    {
        // increment before so we start at frame 1
        recording_image_counter_++;
        std::stringstream filename;
        filename << "frame_" << std::setw(6) << std::setfill('0') << recording_image_counter_ << recording_fileformat_;
        if (!std::filesystem::exists(recordings_path_))
        {
            if (!std::filesystem::create_directory(recordings_path_))
            {
                std::cerr << "Error: failed to create directory to store recordings" << std::endl;
                return;
            }
            std::cout << "Created directory to store recordings at: " << std::filesystem::absolute(recordings_path_)
                      << std::endl;
        }

        std::filesystem::path file = recordings_path_ / filename.str();
        auto time = renderer_.get_itime();
        // allocate buffer

        recording_buffer_used_++;

        int buffer_idx = recording_buffer_used_ - 1;
        // read framebuffer
        glfwMakeContextCurrent(window_);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        // offset in bytes because buffer is vector of unsigned char
        size_t offset = (size_t)buffer_idx * (size_t)width() * (size_t)height() * (size_t)3;
        glReadnPixels(
            0, 0, width(), height(), GL_RGB, GL_UNSIGNED_BYTE, width() * height() * 3, &recording_frame_data_[offset]);

        recording_buffer_threads_.emplace(new std::thread(&Viewer::write_frame_to_file, this, file, buffer_idx));

        // TODO: This is not optimal but good enough (use something like a thread pool instead)
        // if buffers are full, wait until they're completely empty again
        if (recording_buffer_used_ == recording_buffer_count_)
        {
            join_recording_buffer_threads();
            recording_buffer_used_ = 0;
        }
        renderer_.set_itime(time + 1.0 / (double)recording_framerate_);

        if ((recording_frame_target_count_ > 0) && (recording_image_counter_ >= recording_frame_target_count_))
            stop_recording();
    }
}

void Viewer::write_frame_to_file(std::filesystem::path filename, int buffer_idx)
{
    // write to file
    stbi_flip_vertically_on_write(true);
    // offset in bytes because buffer is vector of unsigned char
    size_t offset = (size_t)buffer_idx * (size_t)width() * (size_t)height() * (size_t)3;
    if (recording_fileformat_ == ".png")
    {
        stbi_write_png(filename.c_str(), width(), height(), 3, &recording_frame_data_[offset], 3 * width());
    }
    else
    {
        stbi_write_jpg(filename.c_str(), width(), height(), 3, &recording_frame_data_[offset], 90);
    }
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
    ready_for_display_ = true;
}

std::string Viewer::seconds_to_string(int seconds)
{
    std::stringstream time;
    time << std::setw(2) << std::setfill('0') << std::floor(seconds / 60) << ':' << std::setw(2) << std::setfill('0')
         << (seconds % 60);
    return time.str();
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
    ImGui::StyleColorsDark();

    if (ImGui::CollapsingHeader("Settings"))
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

        if (ImGui::CollapsingHeader("Recording Settings"))
        {
            {
                static int width = 800;
                static int height = 600;
                ImGui::BeginDisabled(recording_);
                ImGui::InputInt("Window Width", &width, 0, 0);
                ImGui::InputInt("Window Height", &height, 0, 0);
                if (ImGui::Button("Set window size"))
                {
                    if (width > 0 && height > 0)
                        glfwSetWindowSize(window_, width, height);
                }
                ImGui::SameLine();
                if (ImGui::Button("720p"))
                {
                    glfwSetWindowSize(window_, 1024, 720);
                }
                ImGui::SameLine();
                if (ImGui::Button("1080p"))
                {
                    glfwSetWindowSize(window_, 1920, 1080);
                }
                ImGui::SameLine();
                ImGui::EndDisabled();

                IMGUI_TOOLTIP_TEXT(
                    "Sets the window size to the specified values (make sure the window is not in fullscreen mode)");
            }

            ImGui::Separator();

            ImGui::BeginDisabled(recording_);
            ImGui::Checkbox("Automatically convert frames to video (requires ffmpeg in path)",
                            &recording_create_video_);
            ImGui::EndDisabled();

            IMGUI_TOOLTIP_TEXT("Attempts to automatically convert the frames to a video after stopping the recording. "
                               "(See console for info on output file)")
            ImGui::BeginDisabled(recording_);
            ImGui::SliderInt("Recording framerate", &recording_framerate_, 1, 200);

            if (ImGui::Button("30 FPs"))
            {
                recording_framerate_ = 30;
            }
            ImGui::SameLine();
            if (ImGui::Button("60 FPs"))
            {
                recording_framerate_ = 60;
            }
            ImGui::SameLine();
            if (ImGui::Button("120 FPS"))
            {
                recording_framerate_ = 120;
            }
            ImGui::SameLine();
            if (ImGui::Button("144 FPS"))
            {
                recording_framerate_ = 144;
            }

            ImGui::EndDisabled();
            IMGUI_TOOLTIP_TEXT("Framerate to record at. Determines the timesteps between the frames when recording.")

            ImGui::BeginDisabled(recording_);
            ImGui::InputInt("Recording target framecount", &recording_frame_target_count_, 0, 0);
            IMGUI_TOOLTIP_TEXT("Amount of frames to record. Use 0 for infinte (you have to manually stop recording)")
            ImGui::SliderInt("Recording target buffer count", &recording_buffer_count_, 1, 5000);
            IMGUI_TOOLTIP_TEXT(
                "Amount of frames to buffer. Increasing this number directly corresponds with higher RAM usage.")
            ImGui::EndDisabled();
            {
                std::stringstream ram_usage;
                size_t bytes = (size_t)width() * (size_t)height() * (size_t)recording_buffer_count_ * (size_t)3;
                ram_usage << "Buffer RAM usage: " << std::fixed << std::setprecision(2)
                          << (bytes / 1024.0 / 1024.0 / 1024.0) << "GB";
                ImGui::Text("%s", ram_usage.str().c_str());
            }

            {
                std::stringstream videotime;
                int seconds = recording_frame_target_count_ / recording_framerate_;
                videotime << "Video length: " << seconds_to_string(seconds);
                ImGui::Text("%s", videotime.str().c_str());
            }

            if (recording_)
            {
                {
                    std::stringstream buffer_used;
                    buffer_used << "Buffer used: " << recording_buffer_used_ << '/' << recording_buffer_count_;
                    ImGui::Text("%s", buffer_used.str().c_str());
                }
                ImGui::Text("Frame: %d/%d", recording_image_counter_, recording_frame_target_count_);
                {
                    std::stringstream recordingtime;
                    int seconds = recording_image_counter_ / recording_framerate_;
                    recordingtime << "Time (Frames): " << seconds_to_string(seconds);
                    ImGui::Text("%s", recordingtime.str().c_str());
                }
                {
                    std::stringstream recordingtime;
                    auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::system_clock::now() - recording_start_time_);
                    recordingtime << "Elapsed Time since recoding: " << seconds_to_string(elapsed_time.count());
                    ImGui::Text("%s", recordingtime.str().c_str());
                }
            }

            if (ImGui::Button(recording_ ? "Stop recording" : "Start recording"))
            {
                if (recording_)
                    stop_recording();
                else
                {
                    if (std::filesystem::exists(recordings_path_ / "frame_000001.png")
                        || std::filesystem::exists(recordings_path_ / "frame_000001.jpg"))
                    {
                        std::cout << "Frame files exist!!" << std::endl;
                        ImGui::OpenPopup("Delete?");
                    }
                    else
                    {
                        start_recording();
                    }
                }
            }
            IMGUI_TOOLTIP_TEXT("Will reset iTime and starts to save every frame to the 'recordings' directory.")

            {
                std::stringstream fileformat;
                fileformat << "Fileformat: " << recording_fileformat_;

                ImGui::SameLine();
                if (ImGui::Button(fileformat.str().c_str()))
                {
                    if (recording_fileformat_ == ".png")
                    {
                        recording_fileformat_ = ".jpg";
                    }
                    else
                    {
                        recording_fileformat_ = ".png";
                    }
                }
                IMGUI_TOOLTIP_TEXT(
                    "Switches the file format used to save each frame. Jpg is smaller but has artifacts.")
            }
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

            if (ImGui::BeginPopupModal("Delete?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text(
                    "Frame files already exist in the output directory. Delete frame files and start recording?");
                ImGui::Separator();
                if (ImGui::Button("Yes", ImVec2(120, 0)))
                {
                    delete_recorded_frames();
                    start_recording();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();
                if (ImGui::Button("No", ImVec2(120, 0)))
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

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
                renderer_.mesh_size_uniform_ = 1.0;
                renderer_.mesh_size_x_ = 1.0;
                renderer_.mesh_size_y_ = 1.0;
                renderer_.mesh_size_z_ = 1.0;
                set_mesh_scale(1.0);
                update_mesh();
            }

            if (ImGui::SliderFloat("Mesh scale Uniformly:", &renderer_.mesh_size_uniform_, 0.01, 40))
            {
                pmp::vec3 scaling = pmp::vec3(
                    renderer_.mesh_size_uniform_, renderer_.mesh_size_uniform_, renderer_.mesh_size_uniform_);
                renderer_.mesh_size_x_ = renderer_.mesh_size_uniform_;
                renderer_.mesh_size_y_ = renderer_.mesh_size_uniform_;
                renderer_.mesh_size_z_ = renderer_.mesh_size_uniform_;
                set_mesh_scale(scaling);
                update_mesh();
            }

            if (ImGui::SliderFloat("Mesh scale X:", &renderer_.mesh_size_x_, 0.01, 40))
            {
                pmp::vec3 scaling = pmp::vec3(renderer_.mesh_size_x_, renderer_.mesh_size_y_, renderer_.mesh_size_z_);
                set_mesh_scale(scaling);
                update_mesh();
            }
            if (ImGui::SliderFloat("Mesh scale Y:", &renderer_.mesh_size_y_, 0.01, 40))
            {
                pmp::vec3 scaling = pmp::vec3(renderer_.mesh_size_x_, renderer_.mesh_size_y_, renderer_.mesh_size_z_);
                set_mesh_scale(scaling);
                update_mesh();
            }
            if (ImGui::SliderFloat("Mesh scale Z:", &renderer_.mesh_size_z_, 0.01, 40))
            {
                pmp::vec3 scaling = pmp::vec3(renderer_.mesh_size_x_, renderer_.mesh_size_y_, renderer_.mesh_size_z_);
                set_mesh_scale(scaling);
                update_mesh();
            }
        }

        ImGui::Spacing();
        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Shader Settings"))
        {
            static float reflectiveness = 0.3f;
            if (ImGui::SliderFloat("Reflectiveness", &reflectiveness, 0.0, 1.0f))
            {
                renderer_.set_reflectiveness(reflectiveness);
            }
            IMGUI_TOOLTIP_TEXT("How much the reflective sphere shader reflects the skybox");

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

            if (ImGui::Button(renderer_.use_picture_cubemap_ ? "Swap cubemap: Picture"
                                                             : "Swap cubemap: Shader cubemap"))
            {
                renderer_.use_picture_cubemap_ = !renderer_.use_picture_cubemap_;
            }
            IMGUI_TOOLTIP_TEXT(
                "(Only applies in 'Skybox' draw mode, swaps between rendered shader texture and debug texture)");

            if (ImGui::Button("Store current cubemap to file"))
            {
                renderer_.store_skybox_to_file_ = !renderer_.store_skybox_to_file_;
            }
            IMGUI_TOOLTIP_TEXT("(Only applies in 'Skybox' draw mode, stores the current cubemap loaded to a file)");

            if (ImGui::Button(renderer_.offset_skybox_ ? "Offset skybox forward (Debug): ON"
                                                       : "Offset skybox forward (Debug): OFF"))
            {
                renderer_.offset_skybox_ = !renderer_.offset_skybox_;
            }
            IMGUI_TOOLTIP_TEXT(
                "(Only applies in 'Skybox' draw mode, offsets the skybox cube forward to look at it from the outside)");

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

            ImGui::ColorEdit3("Front Color", renderer_.front_color_.data());
            ImGui::ColorEdit3("Back Color", renderer_.back_color_.data());
            ImGui::SliderFloat("Ambient", &renderer_.ambient_, 0, 1);
            ImGui::SliderFloat("Diffuse", &renderer_.diffuse_, 0, 1);
            ImGui::SliderFloat("Specular", &renderer_.specular_, 0, 1);
            ImGui::SliderFloat("shininess", &renderer_.shininess_, 0, 200);
            ImGui::SliderFloat("alpha", &renderer_.alpha_, 0, 1);
            ImGui::Checkbox("Use Lighting", &renderer_.use_lighting_);
            ImGui::Checkbox("Use Vertex Color", &renderer_.use_colors_);
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
                ready_for_display_ = true;
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
                if (ImGui::Button("Clear State"))
                {
                    lenia->clear_state();
                    // draw next frame
                    ready_for_display_ = true;
                }
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
                    // draw next frame
                    ready_for_display_ = true;
                }

                if (ImGui::Button("Visualize Kernel Skeleton"))
                {
                    lenia->visualize_kernel_skeleton();
                    // draw next frame
                    ready_for_display_ = true;
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

                {
                    std::stringstream label;
                    if (ImGui::BeginMenu(label.str().c_str()))
                    {
                        if (ImGui::MenuItem("None"))
                            selected_stamp_ = stamps::s_none;
                        if (ImGui::MenuItem("Orbium"))
                            selected_stamp_ = stamps::s_orbium;
                        else if (ImGui::MenuItem("Smiley"))
                            selected_stamp_ = stamps::s_smiley;
                        else if (ImGui::MenuItem("Debug"))
                            selected_stamp_ = stamps::s_debug;
                        else if (ImGui::MenuItem("Geminium"))
                            selected_stamp_ = stamps::s_geminium;
                        else if (ImGui::MenuItem("Gyrorbium"))
                            selected_stamp_ = stamps::s_gyrorbium;
                        else if (ImGui::MenuItem("Velox"))
                            selected_stamp_ = stamps::s_velox;
                        else if (ImGui::MenuItem("Circle"))
                            selected_stamp_ = stamps::s_circle;
                        ImGui::EndMenu();
                    }
                    IMGUI_TOOLTIP_TEXT("Select a stamp and place on a face using the B key");
                }

                if (selected_stamp_ == stamps::s_circle)
                {
                    ImGui::SliderFloat("Inner radius", &stamp_circle_inner_, 0, 50);
                    ImGui::SliderFloat("Outer radius", &stamp_circle_outer_, 0, 50);
                }

                ImGui::Separator();

                // list from where presets can be selected
                static const std::vector<std::string> items
                    = {"Glider Settings", "Geminium Settings", "Gyrorbium Settings", "Velox Settings"};

                for (size_t i = 0; i < items.size(); i++)
                {
                    if (ImGui::Button(items[i].c_str()))
                    {
                        switch (i)
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
                        case 2:
                            lenia->p_mu_ = 0.156;
                            lenia->p_sigma_ = 0.0224;
                            lenia->p_beta_peaks_ = {1};
                            lenia->p_T_ = 10;
                            lenia->p_neighborhood_radius_ = 13 * lenia->average_edge_length_;
                            break;
                        case 3:
                            lenia->p_mu_ = 0.31;
                            lenia->p_sigma_ = 0.048;
                            lenia->p_beta_peaks_ = {0.5, 1};

                            lenia->p_T_ = 10;
                            lenia->p_neighborhood_radius_ = 10 * lenia->average_edge_length_;
                            break;
                        }

                        lenia->allocate_needed_properties();
                    }
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

    if (!ImGui::IsPopupOpen("Creating_Video") && recording_ffmpeg_is_converting_video_)
    {
        ImGui::OpenPopup("Creating_Video");
    }

    if (ImGui::BeginPopupModal("Creating_Video", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Converting frames to video using ffmpeg, see console for info...");

        ImGui::BeginDisabled(recording_ffmpeg_is_converting_video_);
        if (ImGui::Button("Close", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();
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
