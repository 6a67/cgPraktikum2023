#pragma once

#include <pmp/surface_mesh.h>

namespace meshlife
{

/// base class to handle cellular automata on meshes
class MeshAutomaton
{
  public:
    /// Create a base automaton algorithm instance working on \p mesh
    MeshAutomaton(pmp::SurfaceMesh& mesh);

    /// Allocates the properties to store current and last state.
    /// Must be called before initializing the state
    virtual void allocate_needed_properties();

    /// Copy the initial state from an external property \p prop
    void init_state_from_prop(pmp::FaceProperty<float>& prop);

    /// Initialize the state randomly, must be defined when inheriting
    virtual void init_state_random() = 0;

    /// Update the current state by computing \p num_steps timesteps
    virtual void update_state(int num_steps) = 0;

    virtual void set_state(const pmp::Face& f, float value);

    /// Precompute data that is independent of the current state
    /// May be ignored when inheriting, but overriding is recommended for efficient state updates
    virtual void precompute()
    {
    }

    /// Get the state of a cell (= mesh face) \p f
    inline float state(const pmp::Face& f) const
    {
        return state_[f];
    }

    /// Get the constant per-face property that indicates the current state.
    inline const pmp::FaceProperty<float>& state_prop() const
    {
        return state_;
    }


    int p_upper_threshold_ = 3;
    int p_lower_threshold_ = 2;

  protected:
    /// Convenience function that swaps current and last state.
    inline void swap_states()
    {
        std::swap(state_, last_state_);
    }

    pmp::SurfaceMesh& mesh_;
    pmp::FaceProperty<float> state_;      /// The current state for each cell = face
    pmp::FaceProperty<float> last_state_; /// Allows editing current state while reading from unchanged last state
};

} // namespace meshlife
