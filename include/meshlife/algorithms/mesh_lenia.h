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

    float merged_together(const pmp::Face& x);

    /// Returns value of exponential function at r with parameter a
    float exponential_kernel(float r, float a);

    ///
    float exponential_growth(float u, float mu, float sigma);

    float norm_check();

    void place_stamp(pmp::Face f, const std::vector<std::vector<float>>& stamp);

    typedef std::tuple<pmp::Face, float, float> Neighbor;
    typedef std::vector<Neighbor> Neighbors;
    typedef std::vector<Neighbors> NeighborMap;

    void precache_face_values();

    bool is_closed_mesh();

    void kernel_precompute();

    float distance_neighbors(const Neighbor& n);

    float kernel_shell(float r);
    float potential_distribution_u(const pmp::Face& n);

    void highlight_neighbors(pmp::Face& f);

    float kernel_shell_length(const Neighbors& n);
    float kernel_skeleton(float r, const std::vector<float>& beta);
    float k(const Neighbor& n, const Neighbors& neighborhood);
    float growth(float f, float mu, float sigma);

    float p_mu_ = 0.581;
    float p_sigma_ = 0.131;
    float p_neighborhood_radius_ = 0.371;
    int neighbor_count_avg_ = 0;

    std::vector<float> p_beta_peaks_;

    float average_edge_length_ = 0;

    int p_T_ = 10;

  private:
    std::vector<float> kernel_shell_length_;

    NeighborMap neighbor_map_;

    /// Find face with lowest distance to all other facestamp
    pmp::Face find_center_face();

    void initialize_face_map_euclidean();

    void initialize_face_map_geodesic();
};

} // namespace meshlife
