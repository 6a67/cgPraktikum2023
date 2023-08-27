#pragma once

#include <cmath>

#include "pmp/mat_vec.h"
#include "pmp/surface_mesh.h"
#include "pmp/types.h"
#include "pmp/visualization/gl.h"
#include "pmp/visualization/shader.h"

#include <GLFW/glfw3.h>

namespace meshlife
{

inline float degree_to_rad(float degree)
{
    return degree * (M_PI / 180.0);
}

enum class CamDirection
{
    Right,
    Left,
    Top,
    Bottom,
    Front,
    Back,
    COUNT
};

class CustomRenderer
{
  public:
    explicit CustomRenderer(const pmp::SurfaceMesh& mesh, GLFWwindow* window);

    ~CustomRenderer();

    // TODO: Implement these with proper functions or something
    bool use_picture_cubemap_ = false;

    //! Draw the mesh.
    void draw(const pmp::mat4& projection_matrix, const pmp::mat4& modelview_matrix, const std::string& draw_mode);

    //! Update all OpenGL buffers for rendering.
    void update_opengl_buffers();

    void set_simple_shader_files(std::string simple_shader_path_vertex, std::string simple_shader_path_fragment);

    void set_skybox_shader_files(std::string vertex_shader_file_path, std::string fragment_shader_file_path);

    void set_reflective_sphere_shader_files(std::string vertex_shader_file_path, std::string fragment_shader_file_path);

    void set_phong_shader_files(std::string vertex_shader_file_path, std::string fragment_shader_file_path);

    double get_framerate();

    double get_itime();

    void set_itime(double itime = 0.0);

    bool get_itime_paused();

    void itime_pause();

    void itime_continue();

    void itime_toggle_pause();

    CamDirection get_cam_direction();

    void set_cam_direction(CamDirection direction);

    void render_skybox_faces_to_texture(pmp::vec3 model_pos);

    void load_simple_shader();

    void load_skybox_shader();

    void load_reflective_sphere_shader();

    void load_phong_shader();

    void set_reflectiveness(float reflectiveness);

    const std::vector<std::string> direction_names_ = {
        "right",
        "left",
        "top",
        "bottom",
        "front",
        "back",

    };
    const std::vector<pmp::vec3> view_rotations_ = {
        pmp::vec3(0, degree_to_rad(-90), degree_to_rad(0)),
        pmp::vec3(0, degree_to_rad(90), degree_to_rad(0)),
        pmp::vec3(degree_to_rad(-90), 0, degree_to_rad(0)),
        pmp::vec3(degree_to_rad(90), 0, degree_to_rad(0)),
        pmp::vec3(0, degree_to_rad(0), degree_to_rad(0)),
        pmp::vec3(0, degree_to_rad(180), degree_to_rad(0)),
    };

    const std::vector<pmp::vec3> texture_rotations_ = {
        pmp::vec3(0, degree_to_rad(-90), degree_to_rad(180)),
        pmp::vec3(0, degree_to_rad(90), degree_to_rad(180)),
        pmp::vec3(degree_to_rad(90), 0, degree_to_rad(0)),
        pmp::vec3(degree_to_rad(-90), 0, degree_to_rad(0)),
        pmp::vec3(0, degree_to_rad(180), degree_to_rad(180)),
        pmp::vec3(0, degree_to_rad(0), degree_to_rad(180)),
    };

    const std::vector<pmp::vec3> colors_ = {
        pmp::vec3(1.0, 0.0, 0.0),
        pmp::vec3(0.0, 1.0, 0.0),
        pmp::vec3(0.0, 0.0, 1.0),
        pmp::vec3(1.0, 1.0, 0.0),
        pmp::vec3(0.0, 1.0, 1.0),
        pmp::vec3(1.0, 0.0, 1.0),
    };

    // material properties
    pmp::vec3 front_color_, back_color_;
    float ambient_, diffuse_, specular_, shininess_, alpha_;
    bool use_srgb_;
    bool use_colors_;
    float crease_angle_;
    float point_size_;
    float reflectiveness_;
    bool use_lighting_ = true;

    bool store_skybox_to_file_ = false;
    bool offset_skybox_ = false;

    float fovX_ = 90;

    float mesh_size_uniform_ = 0.05f; // 1.0f;
    float mesh_size_x_ = 0.05f;       // 1.0f;
    float mesh_size_y_ = 0.05f;       // 1.0f;
    float mesh_size_z_ = 0.05f;       // 1.0f;

  private:
    const pmp::SurfaceMesh& mesh_;

    GLFWwindow* window_;

    CamDirection cam_direction_ = CamDirection::Front; // start at direction facing forward

    int cubemap_size_ = 256;

    int wsize_ = 800;

    int hsize_ = 600;

    double framerate_ = 0;

    double itime_ = 0;

    bool itime_paused_ = true;

    std::chrono::high_resolution_clock::time_point last_time_;

    std::vector<std::string> skybox_names_
        = {"right.png", "left.png", "top.png", "bottom.png", "front.png", "back.png"};

    float skybox_Vertices_[108] = { // positions
        -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
        1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

        -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
        -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

        1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
        1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};

    // OpenGL buffers
    GLuint MESH_VAO_ = 0;
    GLuint MESH_vertex_buffer_ = 0;
    GLuint MESH_color_buffer_ = 0;
    GLuint MESH_normal_buffer_ = 0;
    GLuint MESH_tex_coord_buffer_ = 0;
    GLuint MESH_edge_buffer_ = 0;
    GLuint MESH_feature_buffer_ = 0;

    GLsizei n_vertices_ = 0;
    GLsizei n_edges_ = 0;
    GLsizei n_triangles_ = 0;
    GLsizei n_features_ = 0;
    bool has_texcoords_ = false;
    bool has_vertex_colors_ = false;

    GLuint skybox_VAO_ = 0;
    GLuint skybox_VBO_ = 0;

    // OpenGL texture
    GLuint g_framebuffer_ = 0;
    GLuint g_cubeTexture_ = 0;
    GLuint g_depthbuffer_ = 0;
    int g_cubeTexUnit_ = 0;
    unsigned int cubemap_texture_ = 0;

    int skybox_img_width_ = 0;
    int skybox_img_height_ = 0;

    // OpenGL shader
    pmp::Shader simple_shader_;
    pmp::Shader skybox_shader_;
    pmp::Shader reflective_sphere_shader_;
    pmp::Shader phong_shader_;

    std::string simple_shader_path_vertex_;
    std::string simple_shader_path_fragment_;

    std::string skybox_vertex_shader_file_path_;
    std::string skybox_fragment_shader_file_path_;

    std::string reflective_sphere_vertex_shader_file_path_;
    std::string reflective_sphere_fragment_shader_file_path_;

    std::string phong_vertex_shader_file_path_;
    std::string phong_fragment_shader_file_path_;

    void draw_face(int face_side, pmp::vec3 model_pos);

    void create_cube_texture_if_not_exist();

    void draw_skybox(pmp::mat4 projection_matrix, pmp::mat4 view_matrix);

    unsigned int load_cubemap(std::vector<std::string> faces);

    // helpers for computing triangulation of a polygon
    struct Triangulation
    {
        Triangulation(pmp::Scalar a = std::numeric_limits<pmp::Scalar>::max(), int s = -1) : area_(a), split_(s)
        {
        }
        pmp::Scalar area_;
        int split_;
    };

    // access triangulation array
    inline Triangulation& triangulation(int start, int end)
    {
        return triangulation_[polygon_valence_ * start + end];
    }

    // table to hold triangulation data
    std::vector<Triangulation> triangulation_;

    // valence of currently triangulated polygon
    unsigned int polygon_valence_;

    // compute squared area of triangle. used for triangulate().
    inline pmp::Scalar area(const pmp::vec3& p0, const pmp::vec3& p1, const pmp::vec3& p2) const
    {
        return sqrnorm(cross(p1 - p0, p2 - p0));
    }

    // reserve n*n array for computing triangulation
    inline void init_triangulation(unsigned int n)
    {
        triangulation_.clear();
        triangulation_.resize(n * n);
        polygon_valence_ = n;
    }

    // triangulate a polygon such that the sum of squared triangle areas is minimized.
    // this prevents overlapping/folding triangles for non-convex polygons.
    void tesselate(const std::vector<pmp::vec3>& points, std::vector<pmp::ivec3>& triangles);
};

} // namespace meshlife
