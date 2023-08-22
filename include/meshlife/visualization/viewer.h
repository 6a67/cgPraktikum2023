#pragma once

#include "meshlife/algorithms/mesh_automaton.h"
#include "meshlife/algorithms/mesh_lenia.h"
#include "meshlife/stamps.h"
#include "meshlife/visualization/custom_meshviewer.h"
#include <bits/chrono.h>
#include <chrono>
#include <ctime>
#include <pmp/stop_watch.h>
#include <stack>
#include <thread>

namespace meshlife
{

struct DebugData
{
    // used to select a face does not always match with the index of face, use face.idx() instead
    int selected_face_idx_ = 0;

    // the currently selected face (may be invalid use face.is_valid())
    pmp::Face face_;
};

const char* PATH_SIMPLE_SHADER_VERTEX_ = "simple.vert";
const char* PATH_SIMPLE_SHADER_FRAGMENT_ = "camera_movement2.frag";

/// pmp::MeshViewer extension to handle visualization of implemented algorithms
class Viewer : public CustomMeshViewer
{
  public:
    /// window constructor
    Viewer(const char* title, int width, int height);

    ~Viewer();

    template <typename T>
    inline void set_automaton()
    {
        // TODO: Free in destructor
        automaton_ = new T(mesh_);

        if (auto* lenia = dynamic_cast<MeshLenia*>(automaton_))
        {
            std::string s;
            for (auto peak : lenia->p_beta_peaks_)
            {
                s += std::to_string(peak) + ",";
            }
            strcpy(peak_string_, s.c_str());
        }
    }

    void set_mesh_properties();

    void reload_shader();

    void on_shutdown();

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

    void read_mesh_from_file(std::string path);

    void on_close_callback() override;

    void write_frame_to_file(std::filesystem::path filename, int buffer_idx);

    void start_recording();

    void stop_recording();

    void join_recording_buffer_threads();

    void delete_recorded_frames();

    void convert_recorded_frames_to_video();

  private:
    MeshAutomaton* automaton_ = nullptr;
    bool simulation_running_ = false;
    char* modelpath_buf_;
    char* peak_string_;
    stamps::Shapes selected_stamp_ = stamps::Shapes::s_none;
    std::chrono::time_point<std::chrono::high_resolution_clock> clock_last_;

    std::filesystem::path recordings_path_;
    int recording_image_counter_ = 0;
    int recording_frame_target_count_ = 0;
    int recording_framerate_ = 60;
    int recording_buffer_count_ = 100;
    std::chrono::time_point<std::chrono::system_clock> recording_start_time_;
    bool recording_create_video_ = false;
    volatile bool recording_ffmpeg_is_converting_video_ = false;
    std::thread thread_ffmpeg_;
    std::vector<unsigned char> recording_frame_data_;
    std::atomic<int> recording_buffer_used_ = 0;
    std::stack<std::thread*> recording_buffer_threads_;
    std::string recording_fileformat_ = ".jpg";

    // Updates per second
    int UPS_ = 30;
    bool unlimited_limit_UPS_ = false;

    double current_UPS_;

    // Debug data
    DebugData debug_data_;

    bool recording_ = false;

    std::thread simulation_thread_;
    void simulation_thread_func();
    std::atomic<bool> ready_for_display_ = false;
    bool uncomplete_updates_ = false;

    pmp::Color hsv_to_rgb(float h, float s, float v);

    void drop(int count, const char** paths) override;
    void after_display() override;

    pmp::Face get_face_under_cursor();

    void retrieve_debug_info_for_selected_face();
    void select_debug_info_face(size_t face_idx);

    std::string seconds_to_string(int seconds);

    void start_simulation(bool single_step = false);
    void stop_simulation();

    void file_watcher_func();
    std::thread file_watcher_thread_;

    std::filesystem::path shaders_path_;

    volatile bool watch_shader_file_ = false;
    void file_watcher_enable();
    void file_watcher_disable();

    std::string selected_shader_path_vertex_;
    std::string selected_shader_path_fragment_;

    std::filesystem::path get_path_from_shader_type(ShaderType shader_type);

    // Stores a pair of the last modified time and the shader type (we dont store the
    // file name because for our simple shader it changes, and for the Skybox and ReflectiveSphere shader its always the
    // same file
    std::vector<std::pair<std::filesystem::file_time_type, ShaderType>> last_modified_shader_files_;

    // list of shader files
    std::vector<std::string> shader_files_fragment_;
};

} // namespace meshlife
