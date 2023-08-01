#include <meshlife/algorithms/mesh_lenia.h>
#include <pmp/algorithms/differential_geometry.h>

namespace meshlife
{

MeshLenia::MeshLenia(pmp::SurfaceMesh& mesh) : MeshAutomaton(mesh)
{
    // allocate_needed_properties();
}

void MeshLenia::initialize_faceMap()
{
    faceMap.reserve(mesh_.faces_size());
    for (auto fa : mesh_.faces())
    {
        std::vector<std::pair<pmp::Face, float>> neighbors;
        for (auto fb : mesh_.faces())
        {
            pmp::Point pa = pmp::centroid(mesh_, fa);
            pmp::Point pb = pmp::centroid(mesh_, fb);

            float dist = pmp::distance(pa, pb);
            if (dist <= p_neighborhood_radius)
            {
                neighbors.push_back(std::make_pair(fb, dist));
            }
        }
        faceMap.push_back(neighbors);
    }
}

void MeshLenia::update_state(int num_steps)
{
    // kernel shell
    for (auto f : mesh_.faces())
    {
        float sum = 0.0f;
        for (auto neighbor : faceMap[f.idx()])
        {
            sum += exponential_kernel(std::modf(neighbor.second, nullptr), 1.0f) * last_state_[neighbor.first];
        }
        // TODO: normalisierung so nicht richtig
        state_[f] = sum / faceMap[f.idx()].size();
    }
}

void MeshLenia::init_state_random(){};

void MeshLenia::set_state(const pmp::Face& f, float value){};

float MeshLenia::exponential_kernel(float r, float a)
{
    return std::exp(a - (a) / (4 * r * (1 - r)));
}

float MeshLenia::exponential_growth(float u, float mu, float sigma)
{
    return 2 * exp(-pow((u - mu), 2) / (2 * pow(sigma, 2))) - 1;
}

} // namespace meshlife