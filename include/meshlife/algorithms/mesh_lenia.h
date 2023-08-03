#pragma once
#include <meshlife/algorithms/mesh_automaton.h>

#include <pmp/surface_mesh.h>

namespace meshlife
{

class MeshLenia : public MeshAutomaton
{
  public:
    MeshLenia(pmp::SurfaceMesh& mesh);

    /// Initialize the state randomly, must be defined when inheriting
    void init_state_random() override;

    /// Update the current state by computing \p num_steps timesteps
    void update_state(int num_steps) override;

    void allocate_needed_properties() override;

    void visualize_kernel_shell();

    void visualize_kernel_skeleton();

    void visualize_potential();

    /// Returns value of exponential function at r with parameter a
    float exponential_kernel(float r, float a);

    ///
    float exponential_growth(float u, float mu, float sigma);

    typedef std::pair<pmp::Face, float> Neighbor;
    typedef std::vector<Neighbor> Neighbors;
    typedef std::vector<Neighbors> NeighborMap;

    void initialize_faceMap();

    float distance_neighbors(Neighbor n);

    float KernelShell(float r);
    float Potential_Distribution_U(pmp::Face n);

    float KernelShell_Length(Neighbors n);
    float KernelSkeleton(float r, std::vector<float> beta);
    float K(Neighbor n, Neighbors neighborhood);
    float Growth(float f, float mu, float sigma);

    float p_mu = 0.581;
    float p_sigma = 0.131;
    float p_neighborhood_radius = 0.371;
    int neighborCountAvg = 0;

    std::vector<float> p_beta_peaks;

  private:
    float delta_x = 1 / p_neighborhood_radius;

    NeighborMap neighborMap;

    /// Find face with lowest distance to all other faces
    pmp::Face find_center_face();
    std::vector<pmp::Face> faces;
};

} // namespace meshlife
