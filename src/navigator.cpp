#include "meshlife/navigator.h"
#include "pmp/mat_vec.h"
#include <stdexcept>

#include <pmp/surface_mesh.h>

namespace meshlife
{

QuadMeshNavigator::QuadMeshNavigator(pmp::SurfaceMesh mesh) : mesh_(mesh)
{
    if (!mesh.is_quad_mesh())
    {
        throw std::runtime_error("Mesh is not a quad mesh");
    }

    current_face_ = *mesh_.faces_begin();
    current_halfedge_ = mesh_.halfedge(current_face_);
}

bool QuadMeshNavigator::move_forward(int steps)
{
    pmp::Halfedge halfedge = current_halfedge_;
    for (int i = 0; i < steps; i++)
    {
        halfedge = mesh_.opposite_halfedge(halfedge);
        if (mesh_.is_boundary(halfedge))
            return false;
        halfedge = mesh_.next_halfedge(mesh_.next_halfedge(halfedge));
    }
    // move to the correct halfedge in same face
    current_halfedge_ = halfedge;
    current_face_ = mesh_.face(current_halfedge_);

    if (on_enter_face_callback_)
        on_enter_face_callback_(current_face_);

    return true;
}

bool QuadMeshNavigator::move_backward(int steps)
{
    pmp::Halfedge halfedge = current_halfedge_;
    for (int i = 0; i < steps; i++)
    {
        halfedge = mesh_.opposite_halfedge(mesh_.next_halfedge(mesh_.next_halfedge(halfedge)));
        if (mesh_.is_boundary(halfedge))
            return false;
    }
    current_halfedge_ = halfedge;
    current_face_ = mesh_.face(current_halfedge_);

    if (on_enter_face_callback_)
        on_enter_face_callback_(current_face_);

    return true;
}

bool QuadMeshNavigator::move_clockwise(int steps)
{
    rotate_clockwise();
    bool result = move_forward(steps);
    rotate_counterclockwise();
    return result;
}

bool QuadMeshNavigator::move_counterclockwise(int steps)
{
    rotate_counterclockwise();
    bool result = move_forward(steps);
    rotate_clockwise();
    return result;
}

void QuadMeshNavigator::push_position()
{
    stack.push(current_halfedge_);
}

void QuadMeshNavigator::pop_position()
{
    if (stack.empty())
        throw std::runtime_error("Cannot pop empty stack");

    current_halfedge_ = stack.top();
    current_face_ = mesh_.face(current_halfedge_);
    stack.pop();
}

void QuadMeshNavigator::rotate_clockwise()
{
    current_halfedge_ = mesh_.prev_halfedge(current_halfedge_);
}

void QuadMeshNavigator::rotate_counterclockwise()
{
    current_halfedge_ = mesh_.next_halfedge(current_halfedge_);
}

bool QuadMeshNavigator::move_to_face(pmp::Face face)
{
    if (!face.is_valid())
    {
        return false;
    }

    current_face_ = face;
    current_halfedge_ = mesh_.halfedge(face);
    return true;
}

pmp::Face QuadMeshNavigator::current_face() const
{
    return current_face_;
}

pmp::Halfedge QuadMeshNavigator::current_halfedge() const
{
    return current_halfedge_;
}

void QuadMeshNavigator::set_on_enter_face_callback(OnEnterFace on_enter_face_callback)
{
    on_enter_face_callback_ = on_enter_face_callback;
}

pmp::Halfedge QuadMeshNavigator::get_north_halfedge()
{
    pmp::Halfedge north_halfedge;
    float current_dot = 0;
    for (auto halfedge : mesh_.halfedges(current_face_))
    {
        auto start = mesh_.from_vertex(halfedge);
        auto end = mesh_.to_vertex(halfedge);
        auto start_pos = mesh_.position(start);
        auto end_pos = mesh_.position(end);

        auto dir = end_pos - start_pos;
        auto left = pmp::Vector<pmp::Scalar, 3>(-1, 0, 0);

        // search halfedge that mostly points to the left (should be the north halfedge)
        float dot = pmp::dot(dir, left);
        // dot is 1 if the vectors are pointing in the same direction, -1 if they are pointing in opposite directions
        // and 0 if perpendicular
        if (dot > current_dot && dot > 0)
        {
            north_halfedge = halfedge;
            current_dot = dot;
        }
    }
    return north_halfedge;
}

} // namespace meshlife
