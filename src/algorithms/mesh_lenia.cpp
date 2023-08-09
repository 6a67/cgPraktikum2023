#include "meshlife/navigator.h"
#include <iostream>
#include <meshlife/algorithms/helpers.h>
#include <meshlife/algorithms/mesh_lenia.h>
#include <pmp/algorithms/differential_geometry.h>
#include <pmp/algorithms/geodesics.h>
#include <pmp/algorithms/utilities.h>
#include <pmp/surface_mesh.h>
#include <set>

namespace meshlife
{

MeshLenia::MeshLenia(pmp::SurfaceMesh& mesh) : MeshAutomaton(mesh)
{
    p_beta_peaks = {1, 1.0 / 3.0};
    allocate_needed_properties();
}

void MeshLenia::allocate_needed_properties()
{
    MeshAutomaton::allocate_needed_properties();
    // if (is_closed_mesh())
    // {
    //     std::cout << "Closed mesh" << std::endl;
    //     initialize_faceMap_geodesic();
    // }
    // else
    // {
    //     std::cout << "Open mesh" << std::endl;
    //     initialize_faceMap();
    // }
    initialize_faceMap();

    kernel_precompute();
}

bool MeshLenia::is_closed_mesh()
{

    for (auto h : mesh_.halfedges())
    {
        if (!mesh_.is_boundary(h))
        {
            return false;
        }
    }

    return true;
}

void MeshLenia::initialize_faceMap_geodesic()
{
    auto t1 = std::chrono::high_resolution_clock::now();
    std::cout << "Started initializing " << std::endl;

    mesh_.garbage_collection();

    neighborMap.clear();
    neighborMap.resize(mesh_.faces_size());

    pmp::SurfaceMesh dual_mesh(mesh_);
    pmp::dual(dual_mesh);
    std::cout << p_neighborhood_radius << std::endl;
#pragma omp parallel for
    for (size_t i = 0; i < mesh_.faces_size(); i++)
    {
        pmp::SurfaceMesh m(dual_mesh);
        pmp::Vertex v(i);

        std::vector<pmp::Vertex> startVertices;
        startVertices.push_back(v);
        std::vector<pmp::Vertex> neighbors;
        pmp::geodesics(m, startVertices, p_neighborhood_radius, std::numeric_limits<int>::max(), &neighbors);

        Neighbors finalNeighbors;

        pmp::VertexProperty<float> distances = m.get_vertex_property<float>("geodesic:distance");
        for (auto n : neighbors)
        {
            // std::cout << "Distance: " << distances[n] << std::endl;
            auto d = distances[n] / p_neighborhood_radius;
            if (d > 1)
            {
                continue;
            }

            finalNeighbors.push_back(std::make_tuple(pmp::Face(n.idx()), d, 0));
        }

        neighborMap[i] = finalNeighbors;
#pragma omp critical
        {
            neighborCountAvg += neighbors.size();
        }
    }
    neighborCountAvg /= neighborMap.size();

    auto t2 = std::chrono::high_resolution_clock::now();

    /* Getting number of milliseconds as an integer. */
    auto ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);

    /* Getting number of milliseconds as a double. */
    std::chrono::duration<double, std::milli> ms_double = t2 - t1;

    std::cout << "Done initializing. Time:" << std::endl;
    std::cout << ms_int.count() << "ms\n";
    std::cout << ms_double.count() << "ms\n";

    averageEdgeLength = pmp::mean_edge_length(mesh_);
}

void MeshLenia::initialize_faceMap()
{
    auto t1 = std::chrono::high_resolution_clock::now();
    std::cout << "Started initializing " << std::endl;

    mesh_.garbage_collection();

    neighborMap.clear();
    neighborMap.resize(mesh_.faces_size());

#pragma omp parallel for
    for (size_t i = 0; i < mesh_.faces_size(); i++)
    {
        // std::cout << i << std::endl;
        const pmp::Face fa = pmp::Face(i);
        std::vector<Neighbor> neighbors;
        neighbors.reserve(50);

        const pmp::Point pa = pmp::centroid(mesh_, fa);

        // #pragma omp parallel for
        for (size_t j = 0; j < mesh_.faces_size(); j++)
        {
            if (i == j)
                continue;

            const pmp::Face fb = pmp::Face(j);
            const pmp::Point pb = pmp::centroid(mesh_, fb);
            const float dist = pmp::distance(pa, pb);

            if (dist <= p_neighborhood_radius)
            {
                const float d = dist / p_neighborhood_radius;
                // std::cout << "Normalized Distance: " << d << std::endl;

                // #pragma omp critical
                neighbors.push_back(std::make_tuple(fb, d, 0));
            }
        }
        // std::cout << "Added " << neighbors.size() << " neighbors to face: " << fa.idx() << std::endl;

        neighborMap[i] = neighbors;
#pragma omp critical
        {
            neighborCountAvg += neighbors.size();
        }
    }
    neighborCountAvg /= neighborMap.size();

    auto t2 = std::chrono::high_resolution_clock::now();

    /* Getting number of milliseconds as an integer. */
    auto ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);

    /* Getting number of milliseconds as a double. */
    std::chrono::duration<double, std::milli> ms_double = t2 - t1;

    std::cout << "Done initializing. Time:" << std::endl;
    std::cout << ms_int.count() << "ms\n";
    std::cout << ms_double.count() << "ms\n";

    averageEdgeLength = pmp::mean_edge_length(mesh_);
}

void MeshLenia::kernel_precompute()
{
    // ----- Kernel Precomputation -----

    kernel_shell_length.clear();
    kernel_shell_length.resize(mesh_.faces_size());

    #pragma omp parallel for
    for (size_t i = 0; i < neighborMap.size(); i++)
    {
        float ksl = 0;

        // std::cout << "Face: " << i << std::endl;
        for (size_t j = 0; j < neighborMap[i].size(); j++)
        {
            float K_n = 0;
            K_n = KernelSkeleton(std::get<1>(neighborMap[i][j]), p_beta_peaks)
                  * pmp::face_area(mesh_, std::get<0>(neighborMap[i][j]));
            std::get<2>(neighborMap[i][j]) = K_n;
            ksl += K_n;
        }
        kernel_shell_length[i] = ksl;
    }
}

void MeshLenia::update_state(int num_steps)
{
    // TODO: include num_steps in calculation

    delta_x = 1 / p_neighborhood_radius;
    for (auto f : mesh_.faces())
        last_state_[f] = state_[f];
    // kernel shell
    // int i = 4;

    #pragma omp parallel for
    for (size_t i = 0; i < mesh_.faces_size(); i++)
    {

        const pmp::Face f = pmp::Face(i);
        float l;
        // l = Potential_Distribution_U(f);
        // std::cout << "Potential_Distribution_U: " << l << std::endl;
        // std::cout << "Merged togehter: " << merged_together(f) << std::endl;
        l = merged_together(f);
        l = Growth(l, p_mu, p_sigma);
        l = last_state_[f] + (1.0 / p_T) * l;
        l = std::clamp<float>(l, 0.0, 1.0);
        state_[f] = l;
    }
}

void MeshLenia::init_state_random()
{
    // make random faces alive
    for (pmp::Face f : mesh_.faces())
    {
        state_[f] = (float)rand() / RAND_MAX;
    }
};

// one possible kernel function K_c
float MeshLenia::exponential_kernel(float r, float a)
{
    return std::exp(a - (a) / (4 * r * (1 - r)));
}

// One possible growth function G
float MeshLenia::exponential_growth(float u, float m, float s)
{
    return 2.0 * exp(-(pow((u - m), 2.0) / (2.0 * pow(s, 2.0)))) - 1.0;
}

float MeshLenia::Growth(float f, float m, float s)
{
    return exponential_growth(f, m, s);
}

float MeshLenia::KernelShell(float r)
{
    return exponential_kernel(r, 4);
}

float MeshLenia::KernelSkeleton(float r, const std::vector<float>& beta)
{
    size_t idx = std::floor(r * beta.size());
    // std::cout << idx << std::endl;
    if (idx > beta.size())
    {
        throw std::out_of_range("MeshLenia::KernelSkeleton - Index for beta out of range");
    }
    float f;
    return beta[idx] * KernelShell(std::modf(beta.size() * r, &f));
}

float MeshLenia::distance_neighbors(const Neighbor& n)
{
    return std::get<1>(n);
}

float MeshLenia::KernelShell_Length(const Neighbors& n)
{
    // TODO: return the correct value here
    float l = 0;
    for (auto neighbor : n)
    {
        l += KernelSkeleton(std::get<1>(neighbor), p_beta_peaks) * pmp::face_area(mesh_, std::get<0>(neighbor));
    }
    return l;
}

float MeshLenia::K(const Neighbor& n, const Neighbors& neighborhood)
{
    float d = distance_neighbors(n);
    return KernelSkeleton(d, p_beta_peaks) / (KernelShell_Length(neighborhood));
}

float MeshLenia::Potential_Distribution_U(const pmp::Face& x)
{
    // TODO: does not seems to be correct
    Neighbors n = neighborMap[x.idx()];
    float sum = 0;
    for (auto neighbor : n)
    {
        // TODO: maybe remove the delta_x * delta_x
        sum += K(neighbor, n) * last_state_[std::get<0>(neighbor)] * pmp::face_area(mesh_, std::get<0>(neighbor));
    }
    return sum;
}

float MeshLenia::merged_together(const pmp::Face& x)
{
    Neighbors n = neighborMap[x.idx()];

    float sum = 0;

    float kernelShellLength = 0;

    for (auto neighbor : n)
    {
        // Load from cache
        float k = std::get<2>(neighbor);
        sum += k * last_state_[std::get<0>(neighbor)];
    }

    kernelShellLength = kernel_shell_length[x.idx()];

    sum = sum / kernelShellLength;
    return sum;
}

pmp::Face MeshLenia::find_center_face()
{
    pmp::Face center_face;
    float max_dist = MAXFLOAT;

    for (auto fa : mesh_.faces())
    {
        float dist = 0;
        for (auto fb : mesh_.faces())
        {
            if (fa == fb)
                continue;

            pmp::Point pa = pmp::centroid(mesh_, fa);
            pmp::Point pb = pmp::centroid(mesh_, fb);

            float d = pmp::distance(pa, pb);

            dist += d;
        }
        if (dist < max_dist)
        {
            max_dist = dist;
            center_face = fa;
        }
    }
    return center_face;
}

void MeshLenia::visualize_kernel_shell()
{
    pmp::Face furthest_face = find_center_face();

    for (auto f : mesh_.faces())
    {
        state_[f] = 0;
        last_state_[f] = 1;
    }

    // give every face a value according to the kernel shell
    for (auto f : mesh_.faces())
    {
        pmp::Point pa = pmp::centroid(mesh_, furthest_face);
        pmp::Point pb = pmp::centroid(mesh_, f);

        float dist = pmp::distance(pa, pb);

        state_[f] = KernelShell(dist / p_neighborhood_radius);
        if (state_[f] > 1)
        {
            state_[f] = 0;
        }
    }
}

void MeshLenia::visualize_kernel_skeleton()
{
    // visualize the kernel together with the peaks
    pmp::Face furthest_face = find_center_face();

    for (auto f : mesh_.faces())
    {
        state_[f] = 0;
        last_state_[f] = 1;
    }

    // give every face a value according to the kernel shell
    for (auto f : neighborMap[furthest_face.idx()])
    {
        state_[std::get<0>(f)] = KernelSkeleton(std::get<1>(f), p_beta_peaks);
    }
}

std::set<pmp::Face> get_neighbors(const pmp::SurfaceMesh& mesh, const pmp::Face& f)
{
    std::set<pmp::Face> neighbors;
    // iterate over halfedges
    for (auto h : mesh.halfedges(f))
    {
        // get the face on the other side of the halfedge
        pmp::Face neighbor = mesh.face(mesh.opposite_halfedge(h));
        neighbors.insert(neighbor);
    }
    return neighbors;
}

void MeshLenia::place_stamp(pmp::Face f, const std::vector<std::vector<float>>& stamp)
{
    bool placement_error = false;
    if (!f.is_valid())
        f = find_center_face();

    QuadMeshNavigator navigator{mesh_};
    if (!navigator.move_to_face(f))
    {
        std::cerr << "Error: Could not navigate to starting face for stamp. Invalid face given." << std::endl;
    }

    auto north_edge = navigator.get_north_halfedge();
    if (north_edge.is_valid())
    {
        while (navigator.current_halfedge() != north_edge)
        {
            navigator.rotate_counterclockwise();
        }
    }
    else
        std::cerr << "Error: Could not orientate towards north." << std::endl;

    for (size_t y = 0; y < stamp.size(); y++)
    {
        navigator.push_position();
        for (size_t x = 0; x < stamp[y].size(); x++)
        {
            // retrieve value from array,
            float value = stamp[y][x];
            state_[navigator.current_face()] = value;
            if (!navigator.move_clockwise())
            {
                placement_error = true;
            };
        }
        // move down
        navigator.pop_position();
        if (!navigator.move_backward())
            placement_error = true;
    }

    if (placement_error)
    {
        std::cerr << "Error: Could not place stamp correctly, run into boundary" << std::endl;
    }
}

float MeshLenia::norm_check()
{
    // checks if the kernel returns 1 if all neighboring faces have a value of 1

    pmp::Face center_face = find_center_face();

    // backup the current state
    std::vector<float> backup_state;
    for (auto f : mesh_.faces())
    {
        backup_state.push_back(state_[f]);
    }

    // set all faces to 1
    for (auto f : mesh_.faces())
    {
        state_[f] = 0.5;
        last_state_[f] = 0.5;
    }

    float result = Potential_Distribution_U(center_face);

    // restore the state
    for (auto f : mesh_.faces())
    {
        state_[f] = backup_state[f.idx()];
        last_state_[f] = backup_state[f.idx()];
    }

    return result;
}

void MeshLenia::highlight_neighbors(pmp::Face& f)
{
    auto neighbors = neighborMap[f.idx()];

    for (auto n : neighbors)
    {
        state_[std::get<0>(n)] = std::get<1>(n);
    }
}
} // namespace meshlife
