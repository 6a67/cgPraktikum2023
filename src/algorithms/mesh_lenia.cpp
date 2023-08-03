#include <algorithm>
#include <cmath>
#include <meshlife/algorithms/mesh_lenia.h>
#include <pmp/algorithms/differential_geometry.h>

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
    initialize_faceMap();
}

void MeshLenia::initialize_faceMap()
{
    neighborMap.clear();
    neighborMap.reserve(mesh_.faces_size());
    for (auto fa : mesh_.faces())
    {
        std::vector<std::pair<pmp::Face, float>> neighbors;
        for (auto fb : mesh_.faces())
        {
            if (fa == fb)
                continue;
            pmp::Point pa = pmp::centroid(mesh_, fa);
            pmp::Point pb = pmp::centroid(mesh_, fb);

            float dist = pmp::distance(pa, pb);

            if (dist <= p_neighborhood_radius)
            {
                float d = dist / p_neighborhood_radius;
                // std::cout << "Normalized Distance: " << d << std::endl;

                neighbors.push_back(std::make_pair(fb, d));
            }
        }
        // std::cout << "Added " << neighbors.size() << " neighbors to face: " << fa.idx() << std::endl;

        neighborMap.push_back(neighbors);
        neighborCountAvg += neighbors.size();
    }
    neighborCountAvg /= neighborMap.size();
}

void MeshLenia::update_state(int num_steps)
{
    // TODO: include num_steps in calculation
    delta_x = 1 / p_neighborhood_radius;
    for (auto f : mesh_.faces())
        last_state_[f] = state_[f];
    // kernel shell
    // int i = 4;
    for (auto f : mesh_.faces())
    {
        float l;
        l = Potential_Distribution_U(f);
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

void MeshLenia::visualize_potential()
{
    // visualize the potential distribution
    pmp::Face furthest_face = find_center_face();

    for (auto f : mesh_.faces())
    {
        state_[f] = 1;
        last_state_[f] = 1;
    }

    // give every face a value according to the kernel shell
    for (auto f : mesh_.faces())
    {
        state_[f] = Potential_Distribution_U(f);
    }
}

} // namespace meshlife
