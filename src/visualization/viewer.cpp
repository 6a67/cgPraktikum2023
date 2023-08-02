#include <algorithm>
#include <filesystem>
#include <pmp/algorithms/decimation.h>
#include <pmp/algorithms/remeshing.h>
#include <pmp/algorithms/shapes.h>
#include <pmp/algorithms/subdivision.h>
#include <pmp/algorithms/utilities.h>

#include <imgui.h>

#include "meshlife/algorithms/mesh_lenia.h"
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

    set_mesh_properties();

    update_mesh();

    modelpath_buf = new char[300];
    for (int i = 0; i < 300; i++)
    {
        modelpath_buf[i] = 0;
    }

    std::string("./../assets/monkey.obj").copy(modelpath_buf, 299);
    modelpath_buf[299] = '\0';
}

Viewer::~Viewer()
{
    delete modelpath_buf;
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

void Viewer::keyboard(int key, int scancode, int action, int mods)
{
    // only process press and repeat action (no release or other)
    if (action != GLFW_PRESS && action != GLFW_REPEAT)
        return;

    switch (key)
    {
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
    if (automaton)
    {
        // automaton->update_state(1);
        for (auto f : mesh_.faces())
        {
            auto state = automaton->state(f);
            float v = std::clamp(1 - state, 0.0f, 1.0f);
            set_face_color(f, pmp::Color{v, v, v});
        }
    }

    if (automaton && a_gol)
    {
        automaton->update_state(1);
    }

    renderer_.update_opengl_buffers();
}

void Viewer::read_mesh_from_file(std::string path)
{
    std::cout << "Reading from: " << path << std::endl;
    std::filesystem::path file{path};
    if (std::filesystem::exists(file))
    {
        pmp::read(mesh_, file);
        set_mesh_properties();
        update_mesh();
    }
    else
    {
        std::cout << "Filepath does not exist!" << std::endl;
    }

    // mesh_.assign(mesh);
}

void Viewer::process_imgui()
{
    // Show mesh info in GUI via parent class
    pmp::MeshViewer::process_imgui();

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
        ImGui::Checkbox("##", &a_gol);

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

    if (auto lenia = dynamic_cast<MeshLenia*>(automaton))
    {
        ImGui::SliderFloat("Mu", &lenia->p_mu, 0, 1);
        ImGui::SliderFloat("Sigma", &lenia->p_sigma, 0, 1);
        // TODO: recalculate neighbors
        ImGui::SliderFloat("Neighborhood Radius", &lenia->p_neighborhood_radius, 0, 1);
        if (ImGui::Button("Recalculate Neighborhood"))
        {
            // TODO: move a_gol to a better place
            a_gol = false;
            lenia->initialize_faceMap();
        }
        ImGui::LabelText("Avg. Neighbor count:", "%d", lenia->neighborCountAvg);

        if (ImGui::Button("Visualize Kernel Shell"))
        {
            lenia->visualize_kernel_shell();
        }

        if (ImGui::Button("Visualize Kernel Skeleton"))
        {
            lenia->visualize_kernel_skeleton();
        }

        if (ImGui::Button("Visualize Potential"))
        {
            lenia->visualize_potential();
        }

        // text input for peaks
        char* peak_string = new char[300];
        // initialize with current peaks
        std::string s;
        for (auto peak : lenia->p_beta_peaks)
        {
            s += std::to_string(peak) + ",";
        }
        strcpy(peak_string, s.c_str());
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
        }
    }
}

void Viewer::set_face_color(pmp::Face& face, pmp::Color color)
{
    auto colorProperty = mesh_.get_face_property<pmp::Color>("f:color");
    // std::cout << colorProperty[face] << std::endl;
    colorProperty[face] = color;
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
