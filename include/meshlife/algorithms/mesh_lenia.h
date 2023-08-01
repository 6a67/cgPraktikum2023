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

    void set_state(const pmp::Face& f, float value) override;

    /// Returns value of exponential function at r with parameter a
    float exponential_kernel(float r, float a);

    ///
    float exponential_growth(float u, float mu, float sigma);

    typedef std::vector<std::vector<std::pair<pmp::Face, float>>> FaceMap;

    void initialize_faceMap();

  private:
    float p_neighborhood_radius = 5;

    float mu = 0.35;
    float sigma = 0.07;

    FaceMap faceMap;
};

} // namespace meshlife
