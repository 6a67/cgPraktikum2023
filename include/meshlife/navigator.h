#pragma once

#include "pmp/surface_mesh.h"
#include <functional>
#include <stack>

namespace meshlife
{

class QuadMeshNavigator
{
  public:
    QuadMeshNavigator(pmp::SurfaceMesh mesh);

    using OnEnterFace = std::function<void(const pmp::Face)>;

    bool move_forward(int steps = 1);
    bool move_backward(int steps = 1);
    bool move_clockwise(int steps = 1);
    bool move_counterclockwise(int steps = 1);

    void rotate_clockwise();
    void rotate_counterclockwise();

    void push_position();
    void pop_position();

    bool move_to_face(pmp::Face face);

    pmp::Face current_face() const;

    void set_on_enter_face_callback(OnEnterFace on_enter_face_callback);

  private:
    pmp::SurfaceMesh mesh_;
    pmp::Face current_face_;
    pmp::Halfedge current_halfedge_;

    OnEnterFace on_enter_face_callback_;

    std::stack<pmp::Halfedge> stack;
};

} // namespace meshlife
