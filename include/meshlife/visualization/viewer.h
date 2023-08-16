#pragma once

#include "meshlife/algorithms/mesh_automaton.h"
#include "meshlife/algorithms/mesh_lenia.h"
#include "meshlife/visualization/custom_meshviewer.h"
#include <chrono>
#include <ctime>
#include <pmp/stop_watch.h>
#include <thread>

namespace meshlife
{

struct DebugData
{
    // used to select a face does not always match with the index of face, use face.idx() instead
    int selected_face_idx = 0;

    // the currently selected face (may be invalid use face.is_valid())
    pmp::Face face;
};

const char* PATH_SIMPLE_SHADER_VERTEX = "simple_color.vert";
const char* PATH_SIMPLE_SHADER_FRAGMENT = "simple_color.frag";

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
        automaton = new T(mesh_);

        if (auto* lenia = dynamic_cast<MeshLenia*>(automaton))
        {
            std::string s;
            for (auto peak : lenia->p_beta_peaks)
            {
                s += std::to_string(peak) + ",";
            }
            strcpy(peak_string, s.c_str());
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

  private:
    MeshAutomaton* automaton = nullptr;
    bool simulation_running = false;
    char* modelpath_buf;
    char* peak_string;
    std::chrono::time_point<std::chrono::high_resolution_clock> clock_last;

    // Updates per second
    int UPS_ = 30;
    bool unlimited_limit_UPS_ = false;

    double current_UPS_;

    // Debug data
    DebugData debug_data;

    std::thread simulation_thread;
    void simulation_thread_func();
    std::atomic<bool> ready_for_display = false;
    bool uncomplete_updates = false;

    pmp::Color hsv_to_rgb(float h, float s, float v);

    void retrieve_debug_info_for_selected_face();
    void select_debug_info_face(size_t face_idx);

    void start_simulation(bool single_step = false);
    void stop_simulation();

    void file_watcher_func();
    std::thread file_watcher_thread;

    std::filesystem::path shaders_path_;

    bool watch_shader_file_ = false;
    void file_watcher_enable();
    void file_watcher_disable();

    std::string selected_shader_path_vertex_;
    std::string selected_shader_path_fragment_;

    enum class ShaderType
    {
        SimpleVert,
        SimpleFrag,
        SkyboxVert,
        SkyboxFrag,
        ReflectiveSphereVert,
        ReflectiveSphereFrag,
        PhongVert,
        PhongFrag,
        COUNT
    };

    std::filesystem::path get_path_from_shader_type(Viewer::ShaderType shader_type);

    // Stores a pair of the last modified time and the shader type (we dont store the
    // file name because for our simple shader it changes, and for the Skybox and ReflectiveSphere shader its always the
    // same file
    std::vector<std::pair<std::filesystem::file_time_type, ShaderType>> last_modified_shader_files_;

    // list of shader files
    std::vector<std::string> shader_files_fragment_;
};

} // namespace meshlife
