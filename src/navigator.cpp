#include "meshlife/navigator.h"
#include <stdexcept>

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

void QuadMeshNavigator::set_on_enter_face_callback(OnEnterFace on_enter_face_callback)
{
    on_enter_face_callback_ = on_enter_face_callback;
}

} // namespace meshlife