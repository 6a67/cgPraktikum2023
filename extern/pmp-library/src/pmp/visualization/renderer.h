// Copyright 2011-2021 the Polygon Mesh Processing Library developers.
// Distributed under a MIT-style license, see LICENSE.txt for details.

#pragma once

#include <chrono>
#include <filesystem>
#include <limits>

#include "pmp/mat_vec.h"
#include "pmp/types.h"
#include "pmp/visualization/gl.h"
#include "pmp/visualization/shader.h"

#include <GLFW/glfw3.h>

namespace pmp
{

inline float degree_to_rad(float degree)
{
    return degree * (M_PI / 180.0);
}

class SurfaceMesh;

//! Class for rendering surface meshes using OpenGL
//! \ingroup visualization
class Renderer
{
  public:
    //! Constructor
    explicit Renderer(const SurfaceMesh& mesh, GLFWwindow* window);

    //! Default destructor, deletes all OpenGL buffers.
    ~Renderer();

    double framerate = 0;

    //! get front color
    const vec3& front_color() const
    {
        return front_color_;
    }
    //! set front color
    void set_front_color(const vec3& color)
    {
        front_color_ = color;
    }

    //! get back color
    const vec3& back_color() const
    {
        return back_color_;
    }
    //! set back color
    void set_back_color(const vec3& color)
    {
        back_color_ = color;
    }

    //! get ambient reflection coefficient
    float ambient() const
    {
        return ambient_;
    }
    //! set ambient reflection coefficient
    void set_ambient(float a)
    {
        ambient_ = a;
    }

    //! get diffuse reflection coefficient
    float diffuse() const
    {
        return diffuse_;
    }
    //! set diffuse reflection coefficient
    void set_diffuse(float d)
    {
        diffuse_ = d;
    }

    //! get specular reflection coefficient
    float specular() const
    {
        return specular_;
    }
    //! set specular reflection coefficient
    void set_specular(float s)
    {
        specular_ = s;
    }

    //! get specular shininess coefficient
    float shininess() const
    {
        return shininess_;
    }
    //! set specular shininess coefficient
    void set_shininess(float s)
    {
        shininess_ = s;
    }

    //! get alpha value for transparent rendering
    float alpha() const
    {
        return alpha_;
    }
    //! set alpha value for transparent rendering
    void set_alpha(float a)
    {
        alpha_ = a;
    }

    //! get crease angle (in degrees) for visualization of sharp edges
    Scalar crease_angle() const
    {
        return crease_angle_;
    }
    //! set crease angle (in degrees) for visualization of sharp edges
    void set_crease_angle(Scalar ca);

    //! get point size for visualization of points
    float point_size() const
    {
        return point_size_;
    }
    //! set point size for visualization of points
    void set_point_size(float ps)
    {
        point_size_ = ps;
    }

    //! \brief Control usage of color information.
    //! \details Either per-vertex or per-face colors can be used. Vertex colors
    //! are only used if the mesh has a per-vertex property of type Color
    //! named \c "v:color". Face colors are only used if the mesh has a per-face
    //! property of type Color named \c "f:color". If set to false, the
    //! default front and back colors are used. Default is \c true.
    //! \note Vertex colors take precedence over face colors.
    void set_use_colors(bool use_colors)
    {
        use_colors_ = use_colors;
    }

    //! Draw the mesh.
    void draw(const mat4& projection_matrix, const mat4& modelview_matrix, const std::string& draw_mode);

    //! Update all OpenGL buffers for rendering.
    void update_opengl_buffers();

    //! Use color map to visualize scalar fields.
    void use_cold_warm_texture();

    //! Use checkerboard texture.
    void use_checkerboard_texture();

    void reload_shaders(std::string custom_shader_path_vertex, std::string custom_shader_path_fragment);

    void set_skybox_shader_files(std::string vertex_shader_file_path, std::string fragment_shader_file_path);

    //! Load texture from file.
    //! \param filename the location and name of the texture
    //! \param format internal format (GL_RGB, GL_RGBA, GL_SRGB8, etc.)
    //! \param min_filter interpolation filter for minification
    //! \param mag_filter interpolation filter for magnification
    //! \param wrap texture coordinates wrap preference
    //! \throw IOException in case of failure to load texture from file
    void load_texture(const char* filename,
                      GLint format = GL_RGB,
                      GLint min_filter = GL_LINEAR_MIPMAP_LINEAR,
                      GLint mag_filter = GL_LINEAR,
                      GLint wrap = GL_CLAMP_TO_EDGE);

    //! Load mat-cap texture from file. The mat-cap will be used
    //! whenever the drawing mode is "Texture". This also means
    //! that you cannot have texture and mat-cap at the same time.
    //! \param filename the location and name of the texture
    //! \sa See src/apps/mview.cpp for an example usage.
    //! \throw IOException in case of failure to load texture from file
    void load_matcap(const char* filename);

    // unsigned int loadCubemap(int width, int height, std::vector<unsigned char*> faces);

    void keyboard(int key, int action);

  private:
    const SurfaceMesh& mesh_;

    GLFWwindow* window_;

    // helpers for computing triangulation of a polygon
    struct Triangulation
    {
        Triangulation(Scalar a = std::numeric_limits<Scalar>::max(), int s = -1) : area(a), split(s)
        {
        }
        Scalar area;
        int split;
    };

    // table to hold triangulation data
    std::vector<Triangulation> triangulation_;

    // valence of currently triangulated polygon
    unsigned int polygon_valence_;

    // reserve n*n array for computing triangulation
    void init_triangulation(unsigned int n)
    {
        triangulation_.clear();
        triangulation_.resize(n * n);
        polygon_valence_ = n;
    }

    // access triangulation array
    Triangulation& triangulation(int start, int end)
    {
        return triangulation_[polygon_valence_ * start + end];
    }

    // compute squared area of triangle. used for triangulate().
    inline Scalar area(const vec3& p0, const vec3& p1, const vec3& p2) const
    {
        return sqrnorm(cross(p1 - p0, p2 - p0));
    }

    // triangulate a polygon such that the sum of squared triangle areas is minimized.
    // this prevents overlapping/folding triangles for non-convex polygons.
    void tesselate(const std::vector<vec3>& points, std::vector<ivec3>& triangles);

    int counter_ = 4; // start at direction facing forward
    std::vector<vec3> view_rotations_;
    std::vector<std::string> direction_names_;
    std::vector<vec3> colors_;

    // OpenGL buffers
    GLuint vertex_array_object_;
    GLuint vertex_buffer_;
    GLuint color_buffer_;
    GLuint normal_buffer_;
    GLuint tex_coord_buffer_;
    GLuint edge_buffer_;
    GLuint feature_buffer_;

    GLuint background_array_object;
    GLuint background_vertex_buffer_;
    void load_custom_shader();
    Shader custom_shader_;
    GLsizei n_quad_;
    std::vector<vec3> quad_vertices;

    void drawFace(int faceSide);
    void CreateCubeTextureIfNotExist();
    GLuint g_cubeTexture = 0;
    GLuint g_depthbuffer = 0;
    GLuint g_framebuffer = 0;
    int g_cubeTexUnit = 0;

    void load_skybox_shader();
    std::string skybox_vertex_shader_file_path_;
    std::string skybox_fragment_shader_file_path_;

    Shader skybox_shader_;
    GLuint skyboxVAO = 0;
    GLuint skyboxVBO = 0;
    unsigned int cubemap_texture = 0;

    int cubemap_size = 256;

    void drawSkybox(mat4 projection_matrix, mat4 view_matrix);
    unsigned int loadCubemap(std::vector<std::string> faces);

    std::vector<std::string> skybox_names = {"right.jpg", "left.jpg", "top.jpg", "bottom.jpg", "front.jpg", "back.jpg"};

    float skyboxVertices[108] = { // positions
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

    std::string custom_shader_path_vertex_;
    std::string custom_shader_path_fragment_;

    int wsize_ = 800;
    int hsize_ = 600;

    float itime = 0;
    std::chrono::high_resolution_clock::time_point last_time;

    // buffer sizes
    GLsizei n_vertices_;
    GLsizei n_edges_;
    GLsizei n_triangles_;
    GLsizei n_features_;
    bool has_texcoords_;
    bool has_vertex_colors_;

    // shaders
    Shader phong_shader_;
    Shader matcap_shader_;

    // material properties
    vec3 front_color_, back_color_;
    float ambient_, diffuse_, specular_, shininess_, alpha_;
    bool use_srgb_;
    bool use_colors_;
    float crease_angle_;
    float point_size_;

    // 1D texture for scalar field rendering
    GLuint texture_;
    enum class TextureMode
    {
        ColdWarm,
        Checkerboard,
        MatCap,
        Other
    } texture_mode_;
};

inline void CheckOpenGLError(const char* stat, const char* fname, int line)
{
    GLenum err = glGetError();

    if (err != GL_NO_ERROR)
    {

        std::cerr << "ERROR (OPENGL): " << gluErrorString(err) << " '" << stat << "'"
                  << " in " << fname << ":" << line << std::endl;
        exit(1);
    }
}

#define GL_CHECK(stat)                                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        stat;                                                                                                          \
        CheckOpenGLError(#stat, __FILE__, __LINE__);                                                                   \
    } while (0)

} // namespace pmp
