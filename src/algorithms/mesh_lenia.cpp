#include <meshlife/algorithms/helpers.h>
#include <meshlife/algorithms/mesh_lenia.h>
#include <pmp/algorithms/differential_geometry.h>
#include <pmp/algorithms/utilities.h>
#include <set>

namespace meshlife
{

MeshLenia::MeshLenia(pmp::SurfaceMesh& mesh) : MeshAutomaton(mesh)
{
    p_beta_peaks = {1, 1.0 / 3.0};
    allocate_needed_properties();
    faces = std::vector<pmp::Face>();
}

void MeshLenia::allocate_needed_properties()
{
    MeshAutomaton::allocate_needed_properties();
    initialize_faceMap();
}

void MeshLenia::initialize_faceMap()
{
    auto t1 = std::chrono::high_resolution_clock::now();
    std::cout << "Started initializing " << std::endl;
    neighborMap.clear();
    neighborMap.resize(mesh_.faces_size());

    faces.clear();
    faces.reserve(mesh_.faces_size());
    for (const pmp::Face face : mesh_.faces())
    {
        faces.push_back(face);
    }

#pragma omp parallel for
    for (size_t i = 0; i < faces.size(); i++)
    {
        // std::cout << i << std::endl;
        const pmp::Face fa = faces[i];
        std::vector<std::pair<pmp::Face, float>> neighbors;
        neighbors.reserve(50);

        const pmp::Point pa = pmp::centroid(mesh_, fa);

        // #pragma omp parallel for
        for (size_t j = 0; j < faces.size(); j++)
        {
            if (i == j)
                continue;

            const pmp::Face fb = faces[j];
            const pmp::Point pb = pmp::centroid(mesh_, fb);
            const float dist = pmp::distance(pa, pb);

            if (dist <= p_neighborhood_radius)
            {
                const float d = dist / p_neighborhood_radius;
                // std::cout << "Normalized Distance: " << d << std::endl;

                // #pragma omp critical
                neighbors.push_back(std::make_pair(fb, d));
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

void MeshLenia::update_state(int num_steps)
{
    // TODO: include num_steps in calculation

    delta_x = 1 / p_neighborhood_radius;
    for (auto f : mesh_.faces())
        last_state_[f] = state_[f];
        // kernel shell
        // int i = 4;

#pragma omp parallel for
    for (size_t i = 0; i < faces.size(); i++)
    {

        const pmp::Face f = faces[i];
        float l;
        // l = Potential_Distribution_U(f);
        // std::cout << "Potential_Distribution_U: " << l << std::endl;
        // std::cout << "Merged togehter: " << merged_together(f) << std::endl;
        l = merged_together(f);
        l = Growth(l, p_mu, p_sigma);
        l = last_state_[f] + l;
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

float MeshLenia::KernelSkeleton(float r, std::vector<float> beta)
{
    size_t idx = std::floor(r * beta.size());

    if (idx > beta.size())
    {
        throw std::out_of_range("MeshLenia::KernelSkeleton - Index for beta out of range");
    }
    float f;
    return beta[idx] * KernelShell(std::modf(beta.size() * r, &f));
}

float MeshLenia::distance_neighbors(Neighbor n)
{
    return n.second;
}

float MeshLenia::KernelShell_Length(Neighbors n)
{
    // TODO: return the correct value here
    float l = 0;
    for (auto neighbor : n)
    {
        l += KernelSkeleton(neighbor.second, p_beta_peaks) * delta_x * delta_x;
    }
    return l;
}

float MeshLenia::K(Neighbor n, Neighbors neighborhood)
{
    float d = distance_neighbors(n);
    return KernelSkeleton(d, p_beta_peaks) / (KernelShell_Length(neighborhood));
}

float MeshLenia::Potential_Distribution_U(pmp::Face x)
{
    // TODO: does not seems to be correct
    Neighbors n = neighborMap[x.idx()];
    float sum = 0;
    for (auto neighbor : n)
    {
        // TODO: maybe remove the delta_x * delta_x
        sum += K(neighbor, n) * last_state_[neighbor.first] * delta_x * delta_x;
    }
    return sum;
}

float MeshLenia::merged_together(pmp::Face x)
{
    Neighbors n = neighborMap[x.idx()];
    float sum = 0;

    float kernelShellLength = 0;

    for (auto neighbor : n)
    {
        float d = distance_neighbors(neighbor);
        sum += KernelSkeleton(d, p_beta_peaks) * last_state_[neighbor.first];

        kernelShellLength += KernelSkeleton(neighbor.second, p_beta_peaks);
    }

    // kernelShellLength = kernelShellLength * delta_x * delta_x;

    sum = sum / kernelShellLength;

    // return sum * delta_x * delta_x;
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
        state_[f.first] = KernelSkeleton(f.second, p_beta_peaks);
    }
}

std::set<pmp::Face> get_neighbors(pmp::SurfaceMesh mesh, pmp::Face f)
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

enum Direction
{
    LEFT,
    RIGHT
};

// TODO: Create Navigator construct to make this easier
void MeshLenia::place_stamp(pmp::Face f, std::vector<std::vector<float>> stamp)
{
    if (!f.is_valid())
        f = find_center_face();

    pmp::Halfedge current_halfedge = mesh_.halfedge(f);
    Direction dir = Direction::RIGHT;
    for (size_t y = 0; y < stamp.size(); y++)
    {
        for (size_t x = 0; x < stamp[y].size(); x++)
        {
            float value = 0;
            // retrieve value from array,
            if (dir == Direction::RIGHT)
                value = stamp[y][x];
            else
                value = stamp[y][stamp[y].size() - x - 1];

            state_[mesh_.face(current_halfedge)] = value;

            if (x != stamp[y].size() - 1)
            {
                // move right or left to next face
                if (dir == Direction::RIGHT)
                {
                    pmp::Halfedge h = current_halfedge;
                    h = mesh_.opposite_halfedge(h);
                    h = mesh_.next_halfedge(h);
                    h = mesh_.next_halfedge(h);
                    current_halfedge = h;
                }
                else
                {
                    pmp::Halfedge h = current_halfedge;
                    h = mesh_.next_halfedge(h);
                    h = mesh_.next_halfedge(h);
                    h = mesh_.opposite_halfedge(h);
                    current_halfedge = h;
                }
            }
        }
        // move down
        pmp::Halfedge h = current_halfedge;
        h = mesh_.next_halfedge(h);
        h = mesh_.next_halfedge(h);
        h = mesh_.next_halfedge(h);
        h = mesh_.opposite_halfedge(h);
        h = mesh_.next_halfedge(h);
        h = mesh_.next_halfedge(h);
        h = mesh_.next_halfedge(h);
        current_halfedge = h;

        if (dir == Direction::RIGHT)
            dir = Direction::LEFT;
        else
            dir = Direction::RIGHT;
    }
}

} // namespace meshlife
