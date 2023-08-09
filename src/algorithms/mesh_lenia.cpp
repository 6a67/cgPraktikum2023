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
    precache_face_values();
}

bool MeshLenia::is_closed_mesh()
{
    for (auto h : mesh_.halfedges())
    {
        if (mesh_.is_boundary(h))
        {
            return false;
        }
    }
    return true;
}

void MeshLenia::initialize_faceMap_geodesic()
{
    pmp::SurfaceMesh dual_mesh(mesh_);
    pmp::dual(dual_mesh);

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
}

void MeshLenia::initialize_faceMap_euclidean()
{
#pragma omp parallel for
    for (size_t i = 0; i < mesh_.faces_size(); i++)
    {
        const pmp::Face face = pmp::Face(i);
        std::vector<Neighbor> neighbors;
        neighbors.reserve(50);

        const pmp::Point face_pos = pmp::centroid(mesh_, face);

        for (size_t j = 0; j < mesh_.faces_size(); j++)
        {
            if (i == j)
                continue;

            const pmp::Face neighbor = pmp::Face(j);
            const pmp::Point neighbor_pos = pmp::centroid(mesh_, neighbor);
            const float dist = pmp::distance(face_pos, neighbor_pos);

            if (dist <= p_neighborhood_radius)
            {
                const float distance = dist / p_neighborhood_radius;
                neighbors.push_back(std::make_tuple(neighbor, distance, 0));
            }
        }
        neighborMap[i] = neighbors;
#pragma omp critical
        {
            neighborCountAvg += neighbors.size();
        }
    }
    neighborCountAvg /= neighborMap.size();
}

void MeshLenia::precache_face_values()
{
    auto time_start = std::chrono::high_resolution_clock::now();
    std::cout << "Caching values for faster simulation..." << std::endl;

    mesh_.garbage_collection();

    neighborMap.clear();
    neighborMap.resize(mesh_.faces_size());

    if (is_closed_mesh())
    {
        std::cout << "Detected closed mesh. Using geodesic calculation." << std::endl;
        initialize_faceMap_geodesic();
    }
    else
    {
        std::cout << "Detected open mesh. Using euclidean calculation." << std::endl;
        initialize_faceMap_euclidean();
    }
    kernel_precompute();

    auto time_end = std::chrono::high_resolution_clock::now();

    /* Getting number of milliseconds as an integer. */
    auto ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start);

    /* Getting number of milliseconds as a double. */
    std::chrono::duration<double, std::milli> ms_double = time_end - time_start;

    std::cout << "Done initializing. Time:" << std::endl;
    std::cout << ms_int.count() << "ms\n";
    std::cout << ms_double.count() << "ms\n";

    averageEdgeLength = pmp::mean_edge_length(mesh_);
}

void MeshLenia::kernel_precompute()
{
    // ----- Kernel Precomputation -----

    kernel_shell_length_.clear();
    kernel_shell_length_.resize(mesh_.faces_size());

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
        kernel_shell_length_[i] = ksl;
    }
}

void MeshLenia::update_state(int num_steps)
{
    for (int step = 0; step < num_steps; step++)
    {
        for (auto f : mesh_.faces())
            last_state_[f] = state_[f];

#pragma omp parallel for
        for (size_t i = 0; i < mesh_.faces_size(); i++)
        {
            const pmp::Face face = pmp::Face(i);
            float new_state;
            new_state = merged_together(face);
            new_state = Growth(new_state, p_mu, p_sigma);
            new_state = last_state_[face] + (1.0 / p_T) * new_state;
            new_state = std::clamp<float>(new_state, 0.0, 1.0);
            state_[face] = new_state;
        }
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
    Neighbors n = neighborMap[x.idx()];
    float sum = 0;
    for (auto neighbor : n)
    {
        sum += K(neighbor, n) * last_state_[std::get<0>(neighbor)] * pmp::face_area(mesh_, std::get<0>(neighbor));
    }
    return sum;
}

float MeshLenia::merged_together(const pmp::Face& x)
{
    Neighbors n = neighborMap[x.idx()];
    float sum = 0;

    for (auto neighbor : n)
    {
        // Load from cache
        float k = std::get<2>(neighbor);
        sum += k * last_state_[std::get<0>(neighbor)];
    }

    sum = sum / kernel_shell_length_[x.idx()];
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

// TODO: Add option for custom rotation
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
