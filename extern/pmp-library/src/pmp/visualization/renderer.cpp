// Copyright 2011-2021 the Polygon Mesh Processing Library developers.
// Distributed under a MIT-style license, see LICENSE.txt for details.

#include "pmp/visualization/renderer.h"

#include <numbers>
#include <stb_image.h>

#include "pmp/algorithms/normals.h"
#include "pmp/exceptions.h"
#include "pmp/mat_vec.h"
#include "pmp/visualization/cold_warm_texture.h"
#include "pmp/visualization/mat_cap_shader.h"
#include "pmp/visualization/phong_shader.h"

namespace pmp
{

Renderer::Renderer(const SurfaceMesh& mesh, GLFWwindow* window) : mesh_(mesh), window_(window)
{
    // initialize GL buffers to zero
    vertex_array_object_ = 0;
    vertex_buffer_ = 0;
    color_buffer_ = 0;
    normal_buffer_ = 0;
    tex_coord_buffer_ = 0;
    edge_buffer_ = 0;
    feature_buffer_ = 0;

    background_array_object = 0;
    background_vertex_buffer_ = 0;

    // initialize buffer sizes
    n_vertices_ = 0;

    n_edges_ = 0;
    n_triangles_ = 0;
    n_features_ = 0;
    has_texcoords_ = false;
    has_vertex_colors_ = false;

    // material parameters
    front_color_ = vec3(0.6, 0.6, 0.6);
    back_color_ = vec3(0.5, 0.0, 0.0);
    ambient_ = 0.1;
    diffuse_ = 0.8;
    specular_ = 0.6;
    shininess_ = 100.0;
    alpha_ = 1.0;
    use_srgb_ = false;
    use_colors_ = true;
    crease_angle_ = 180.0;
    point_size_ = 5.0;

    // initialize texture
    texture_ = 0;
    texture_mode_ = TextureMode::Other;

    view_directions_ = {
        vec3(0, degree_to_rad(90), 0),
        vec3(0, degree_to_rad(-90), 0),
        vec3(degree_to_rad(90), 0, 0),
        vec3(degree_to_rad(-90), 0, 0),
        vec3(0, degree_to_rad(0), 0),
        vec3(0, degree_to_rad(180), 0),
    };
}

Renderer::~Renderer()
{
    // delete OpenGL buffers
    GL_CHECK(glDeleteBuffers(1, &vertex_buffer_));
    GL_CHECK(glDeleteBuffers(1, &color_buffer_));
    GL_CHECK(glDeleteBuffers(1, &normal_buffer_));
    GL_CHECK(glDeleteBuffers(1, &tex_coord_buffer_));
    GL_CHECK(glDeleteBuffers(1, &edge_buffer_));
    GL_CHECK(glDeleteBuffers(1, &feature_buffer_));
    GL_CHECK(glDeleteVertexArrays(1, &background_array_object));
    GL_CHECK(glDeleteVertexArrays(1, &vertex_array_object_));

    GL_CHECK(glDeleteFramebuffers(1, &g_framebuffer));
    GL_CHECK(glDeleteBuffers(1, &g_depthbuffer));
}

void Renderer::load_texture(const char* filename, GLint format, GLint min_filter, GLint mag_filter, GLint wrap)
{
#ifdef __EMSCRIPTEN__
    // emscripen/WebGL does not like mapmapping for SRGB textures
    if ((min_filter == GL_NEAREST_MIPMAP_NEAREST || min_filter == GL_NEAREST_MIPMAP_LINEAR
         || min_filter == GL_LINEAR_MIPMAP_NEAREST || min_filter == GL_LINEAR_MIPMAP_LINEAR)
        && (format == GL_SRGB8))
        min_filter = GL_LINEAR;
#endif

    // choose number of components (RGB or RGBA) based on format
    int load_components;
    GLint load_format;
    switch (format)
    {
    case GL_RGB:
    case GL_SRGB8:
        load_components = 3;
        load_format = GL_RGB;
        break;

    case GL_RGBA:
    case GL_SRGB8_ALPHA8:
        load_components = 4;
        load_format = GL_RGBA;
        break;

    default:
        load_components = 3;
        load_format = GL_RGB;
    }

    // load with stb_image
    int width, height, n;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* img = stbi_load(filename, &width, &height, &n, load_components);
    if (!img)
        throw IOException("Failed to load texture file: " + std::string(filename));

    // delete old texture
    glDeleteTextures(1, &texture_);

    // setup new texture
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);

    // upload texture data
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, load_format, GL_UNSIGNED_BYTE, img);

    // compute mipmaps
    if (min_filter == GL_LINEAR_MIPMAP_LINEAR)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    // set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

    // use SRGB rendering?
    use_srgb_ = (format == GL_SRGB8);

    // free memory
    stbi_image_free(img);

    texture_mode_ = TextureMode::Other;
}

void Renderer::load_matcap(const char* filename)
{
    try
    {
        load_texture(filename, GL_RGBA, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
    }
    catch (const IOException&)
    {
        throw;
    }
    texture_mode_ = TextureMode::MatCap;
}

void Renderer::use_cold_warm_texture()
{
    if (texture_mode_ != TextureMode::ColdWarm)
    {
        // delete old texture
        glDeleteTextures(1, &texture_);

        // setup new texture
        glGenTextures(1, &texture_);
        glBindTexture(GL_TEXTURE_2D, texture_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, cold_warm_texture.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        use_srgb_ = false;
        texture_mode_ = TextureMode::ColdWarm;
    }
}

void Renderer::use_checkerboard_texture()
{
    if (texture_mode_ != TextureMode::Checkerboard)
    {
        // delete old texture
        glDeleteTextures(1, &texture_);

        // generate checkerboard-like image
        const unsigned int res = 512;
        auto* tex = new GLubyte[res * res * 3];
        GLubyte* tp = tex;
        for (unsigned int x = 0; x < res; ++x)
        {
            for (unsigned int y = 0; y < res; ++y)
            {
                if (((x & 0x20) == 0) ^ ((y & 0x20) == 0))
                {
                    *(tp++) = 42;
                    *(tp++) = 157;
                    *(tp++) = 223;
                }
                else
                {
                    *(tp++) = 255;
                    *(tp++) = 255;
                    *(tp++) = 255;
                }
            }
        }

        // generate texture
        glGenTextures(1, &texture_);
        glBindTexture(GL_TEXTURE_2D, texture_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, res, res, 0, GL_RGB, GL_UNSIGNED_BYTE, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // clean up
        delete[] tex;

        use_srgb_ = false;
        texture_mode_ = TextureMode::Checkerboard;
    }
}

void Renderer::load_custom_shader()
{
    if (custom_shader_path_vertex_ == "" || custom_shader_path_fragment_ == "")
    {

        std::cerr << "Error: No shader path provided. Did you call Renderer::reload_shaders?" << std::endl;
    }
    else
    {
        try
        {
            custom_shader_.load(custom_shader_path_vertex_.c_str(), custom_shader_path_fragment_.c_str());
        }
        catch (GLException& e)
        {
            std::cerr << "Error: loading custom shader failed" << std::endl;
            std::cerr << e.what() << std::endl;
        }
    }
}

void Renderer::load_texture_shader()
{
    try
    {
        texture_shader_.load("../src/shaders/passthrough.vert", "../src/shaders/texture.frag");
    }
    catch (GLException& e)
    {
        std::cerr << "Error: loading texture shader failed" << std::endl;
        std::cerr << e.what() << std::endl;
    }
}

void Renderer::set_crease_angle(Scalar ca)
{
    if (ca != crease_angle_)
    {
        crease_angle_ = std::max(Scalar(0), std::min(Scalar(180), ca));
        update_opengl_buffers();
    }
}

void Renderer::reload_shaders(std::string custom_shader_path_vertex, std::string custom_shader_path_fragment)
{
    custom_shader_path_fragment_ = custom_shader_path_fragment;
    custom_shader_path_vertex_ = custom_shader_path_vertex;

    // load automatically cleans up the old shader
    load_custom_shader();
}

void Renderer::update_opengl_buffers()
{
    glfwGetWindowSize(window_, &wsize_, &hsize_);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFBO);

    // get time in seconds
    itime = glfwGetTime();

    if (!g_framebuffer)
    {
        std::cout << "Creating Framebuffer" << std::endl;
        GL_CHECK(glGenFramebuffers(1, &g_framebuffer));
        // GL_CHECK(glGenBuffers(1, &g_depthbuffer));
        // std::cout << "ID : " << FramebufferNames_ << std::endl;

        // GL_CHECK(glGenTextures(6, renderedTexture_));
    }

    // are buffers already initialized?
    if (!vertex_array_object_)
    {
        GL_CHECK(glGenVertexArrays(1, &vertex_array_object_));
        GL_CHECK(glBindVertexArray(vertex_array_object_));
        GL_CHECK(glGenBuffers(1, &vertex_buffer_));
        GL_CHECK(glGenBuffers(1, &color_buffer_));
        GL_CHECK(glGenBuffers(1, &normal_buffer_));
        GL_CHECK(glGenBuffers(1, &tex_coord_buffer_));
        GL_CHECK(glGenBuffers(1, &edge_buffer_));
        GL_CHECK(glGenBuffers(1, &feature_buffer_));
    }

    if (!background_array_object)
    {
        GL_CHECK(glGenVertexArrays(1, &background_array_object));
    }

    // activate VAO
    GL_CHECK(glBindVertexArray(vertex_array_object_));

    // get properties
    auto vpos = mesh_.get_vertex_property<Point>("v:point");
    auto vcolor = mesh_.get_vertex_property<Color>("v:color");
    auto vtex = mesh_.get_vertex_property<TexCoord>("v:tex");
    auto htex = mesh_.get_halfedge_property<TexCoord>("h:tex");
    auto fcolor = mesh_.get_face_property<Color>("f:color");

    // index array for remapping vertex indices during duplication
    std::vector<size_t> vertex_indices(mesh_.n_vertices());

    // produce arrays of points, normals, and texcoords
    // (duplicate vertices to allow for flat shading)
    std::vector<vec3> position_array;
    std::vector<vec3> color_array;
    std::vector<vec3> normal_array;
    std::vector<vec2> tex_array;
    std::vector<ivec3> triangles;

    // we have a mesh: fill arrays by looping over faces
    if (mesh_.n_faces())
    {
        // reserve memory
        position_array.reserve(3 * mesh_.n_faces());
        normal_array.reserve(3 * mesh_.n_faces());
        if (htex || vtex)
            tex_array.reserve(3 * mesh_.n_faces());

        if ((vcolor || fcolor) && use_colors_)
            color_array.reserve(3 * mesh_.n_faces());

        // precompute normals for easy cases
        std::vector<Normal> face_normals;
        std::vector<Normal> vertex_normals;
        face_normals.reserve(mesh_.n_faces());
        vertex_normals.reserve(mesh_.n_vertices());
        if (crease_angle_ < 1)
        {
            for (auto f : mesh_.faces())
                face_normals.emplace_back(face_normal(mesh_, f));
        }
        else if (crease_angle_ > 170)
        {
            for (auto v : mesh_.vertices())
                vertex_normals.emplace_back(vertex_normal(mesh_, v));
        }

        // data per face (for all corners)
        std::vector<Halfedge> corner_halfedges;
        std::vector<Vertex> corner_vertices;
        std::vector<vec3> corner_positions;
        std::vector<vec3> corner_colors;
        std::vector<vec3> corner_normals;
        std::vector<vec2> corner_texcoords;

        // convert from degrees to radians
        const Scalar crease_angle_radians = crease_angle_ / 180.0 * M_PI;

        size_t vidx(0);

        // loop over all faces
        for (auto f : mesh_.faces())
        {
            // collect corner positions and normals
            corner_halfedges.clear();
            corner_vertices.clear();
            corner_positions.clear();
            corner_colors.clear();
            corner_normals.clear();
            corner_texcoords.clear();
            Vertex v;
            Normal n;

            for (auto h : mesh_.halfedges(f))
            {
                v = mesh_.to_vertex(h);
                corner_halfedges.push_back(h);
                corner_vertices.push_back(v);
                corner_positions.push_back((vec3)vpos[v]);

                if (crease_angle_ < 1)
                {
                    n = face_normals[f.idx()];
                }
                else if (crease_angle_ > 170)
                {
                    n = vertex_normals[v.idx()];
                }
                else
                {
                    n = corner_normal(mesh_, h, crease_angle_radians);
                }
                corner_normals.push_back((vec3)n);

                if (htex)
                {
                    corner_texcoords.push_back((vec2)htex[h]);
                }
                else if (vtex)
                {
                    corner_texcoords.push_back((vec2)vtex[v]);
                }

                if (vcolor && use_colors_)
                {
                    corner_colors.push_back((vec3)vcolor[v]);
                }
                else if (fcolor && use_colors_)
                {
                    corner_colors.push_back((vec3)fcolor[f]);
                }
            }
            assert(corner_vertices.size() >= 3);

            // tessellate face into triangles
            tesselate(corner_positions, triangles);
            for (auto& t : triangles)
            {
                int i0 = t[0];
                int i1 = t[1];
                int i2 = t[2];

                position_array.push_back(corner_positions[i0]);
                position_array.push_back(corner_positions[i1]);
                position_array.push_back(corner_positions[i2]);

                normal_array.push_back(corner_normals[i0]);
                normal_array.push_back(corner_normals[i1]);
                normal_array.push_back(corner_normals[i2]);

                if (htex || vtex)
                {
                    tex_array.push_back(corner_texcoords[i0]);
                    tex_array.push_back(corner_texcoords[i1]);
                    tex_array.push_back(corner_texcoords[i2]);
                }

                if ((vcolor || fcolor) && use_colors_)
                {
                    color_array.push_back(corner_colors[i0]);
                    color_array.push_back(corner_colors[i1]);
                    color_array.push_back(corner_colors[i2]);
                }

                vertex_indices[corner_vertices[i0].idx()] = vidx++;
                vertex_indices[corner_vertices[i1].idx()] = vidx++;
                vertex_indices[corner_vertices[i2].idx()] = vidx++;
            }
        }
    }

    // we have a point cloud
    else if (mesh_.n_vertices())
    {
        auto position = mesh_.get_vertex_property<Point>("v:point");
        if (position)
        {
            position_array.reserve(mesh_.n_vertices());
            for (auto v : mesh_.vertices())
                position_array.push_back((vec3)position[v]);
        }

        auto normals = mesh_.get_vertex_property<Point>("v:normal");
        if (normals)
        {
            normal_array.reserve(mesh_.n_vertices());
            for (auto v : mesh_.vertices())
                normal_array.push_back((vec3)normals[v]);
        }

        if (vcolor && use_colors_)
        {
            color_array.reserve(mesh_.n_vertices());
            for (auto v : mesh_.vertices())
                color_array.push_back((vec3)vcolor[v]);
        }
    }

    // upload vertices
    if (!position_array.empty())
    {
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_));
        GL_CHECK(glBufferData(
            GL_ARRAY_BUFFER, position_array.size() * 3 * sizeof(float), position_array.data(), GL_STATIC_DRAW));
        GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr));
        GL_CHECK(glEnableVertexAttribArray(0));
        n_vertices_ = position_array.size();
    }
    else
    {
        GL_CHECK(glDisableVertexAttribArray(0));
        n_vertices_ = 0;
    }

    // upload normals
    if (!normal_array.empty())
    {
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, normal_buffer_));
        GL_CHECK(glBufferData(
            GL_ARRAY_BUFFER, normal_array.size() * 3 * sizeof(float), normal_array.data(), GL_STATIC_DRAW));
        GL_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr));
        GL_CHECK(glEnableVertexAttribArray(1));
    }
    else
    {
        GL_CHECK(glDisableVertexAttribArray(1));
    }

    // upload texture coordinates
    if (!tex_array.empty())
    {
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, tex_coord_buffer_));
        GL_CHECK(glBufferData(GL_ARRAY_BUFFER, tex_array.size() * 2 * sizeof(float), tex_array.data(), GL_STATIC_DRAW));
        GL_CHECK(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr));
        GL_CHECK(glEnableVertexAttribArray(2));
        has_texcoords_ = true;
    }
    else
    {
        GL_CHECK(glDisableVertexAttribArray(2));
        has_texcoords_ = false;
    }

    // upload colors of vertices
    if (!color_array.empty())
    {
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, color_buffer_));
        GL_CHECK(
            glBufferData(GL_ARRAY_BUFFER, color_array.size() * 3 * sizeof(float), color_array.data(), GL_STATIC_DRAW));
        GL_CHECK(glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, nullptr));
        GL_CHECK(glEnableVertexAttribArray(3));
        has_vertex_colors_ = true;
    }
    else
    {
        GL_CHECK(glDisableVertexAttribArray(3));
        has_vertex_colors_ = false;
    }

    // edge indices
    if (mesh_.n_edges())
    {
        std::vector<unsigned int> edge_indices;
        edge_indices.reserve(mesh_.n_edges());

        for (auto e : mesh_.edges())
        {
            auto v0 = mesh_.vertex(e, 0).idx();
            auto v1 = mesh_.vertex(e, 1).idx();
            edge_indices.push_back(vertex_indices[v0]);
            edge_indices.push_back(vertex_indices[v1]);
        }

        GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, edge_buffer_));
        GL_CHECK(glBufferData(
            GL_ELEMENT_ARRAY_BUFFER, edge_indices.size() * sizeof(unsigned int), edge_indices.data(), GL_STATIC_DRAW));
        n_edges_ = edge_indices.size();
    }
    else
        n_edges_ = 0;

    // feature edges
    auto efeature = mesh_.get_edge_property<bool>("e:feature");
    if (efeature)
    {
        std::vector<unsigned int> features;

        for (auto e : mesh_.edges())
        {
            if (efeature[e])
            {
                auto v0 = mesh_.vertex(e, 0).idx();
                auto v1 = mesh_.vertex(e, 1).idx();
                features.push_back(vertex_indices[v0]);
                features.push_back(vertex_indices[v1]);
            }
        }

        GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, feature_buffer_));
        GL_CHECK(glBufferData(
            GL_ELEMENT_ARRAY_BUFFER, features.size() * sizeof(unsigned int), features.data(), GL_STATIC_DRAW));
        n_features_ = features.size();
    }
    else
        n_features_ = 0;

    GL_CHECK(glBindVertexArray(0));

    GL_CHECK(glBindVertexArray(background_array_object));

    // unbind object
    GL_CHECK(glBindVertexArray(0));
}

void Renderer::keyboard(int key, int action)
{
    if (action != GLFW_PRESS && action != GLFW_REPEAT)
        return;

    switch (key)

    {
    case GLFW_KEY_C:
        counter_++;
        counter_ = counter_ % 6;
        std::cout << counter_ << " - " << view_directions_[counter_] << std::endl;
    }
}

void Renderer::CreateCubeTexture()
{
    if (g_cubeTexture)
    {
        return;
    }

    GL_CHECK(glGenTextures(1, &g_cubeTexture));
    GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, g_cubeTexture));

    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0));
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0));
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR));

    int CUBE_TEXTURE_SIZE = 1000;
    std::vector<GLubyte> testData(CUBE_TEXTURE_SIZE * CUBE_TEXTURE_SIZE * 256, 128);
    std::vector<GLubyte> xData(CUBE_TEXTURE_SIZE * CUBE_TEXTURE_SIZE * 256, 255);

    for (int loop = 0; loop < 6; ++loop)
    {
        if (loop)
        {
            GL_CHECK(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + loop,
                                  0,
                                  GL_RGBA8,
                                  CUBE_TEXTURE_SIZE,
                                  CUBE_TEXTURE_SIZE,
                                  0,
                                  GL_RGBA,
                                  GL_UNSIGNED_BYTE,
                                  &testData[0]));
        }
        else
        {
            GL_CHECK(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + loop,
                                  0,
                                  GL_RGBA8,
                                  CUBE_TEXTURE_SIZE,
                                  CUBE_TEXTURE_SIZE,
                                  0,
                                  GL_RGBA,
                                  GL_UNSIGNED_BYTE,
                                  &xData[0]));
        }
    }

    GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
}

void Screendump(const char* tga_file, GLuint framebuffer, short W, short H)
{
    FILE* out = fopen(tga_file, "w");
    char pixel_data[3 * W * H];
    short TGAhead[] = {0, 2, 0, 0, 0, 0, W, H, 24};

    glNamedFramebufferReadBuffer(framebuffer, GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, W, H, GL_BGR, GL_UNSIGNED_BYTE, pixel_data);
    fwrite(&TGAhead, sizeof(TGAhead), 1, out);
    fwrite(pixel_data, 3 * W * H, 1, out);
    fclose(out);
}

void Renderer::drawFace(int face_side)
{
    GL_CHECK(glFramebufferTexture2D(
        GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face_side, g_cubeTexture, 0));

    GLenum status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        printf("Status error: %08x\n", status);

    GL_CHECK(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
    GL_CHECK(glClearDepth(1.0f));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    // Render on the whole framebuffer, complete from the lower left corner to the upper right corner
    GL_CHECK(glViewport(0, 0, 1000, 1000));

    custom_shader_.use();
    custom_shader_.set_uniform("window_height", 1000);
    custom_shader_.set_uniform("window_width", 1000);
    custom_shader_.set_uniform("iTime", itime);
    custom_shader_.set_uniform("viewRotation", view_directions_[counter_]);

    vec3 origin = vec3(0, 0, 0);
    custom_shader_.set_uniform("origin", origin);

    GLuint empty_vao = 0;
    GL_CHECK(glGenVertexArrays(1, &empty_vao));
    GL_CHECK(glBindVertexArray(empty_vao));

    GL_CHECK(glActiveTexture(GL_TEXTURE0 + g_cubeTexUnit));
    GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, g_cubeTexture));

    GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 3));

    // doesnt seem to work as expected
    // std::stringstream ss;
    // ss << "test_img_" << face_side << ".tga";
    // Screendump(ss.str().c_str(), g_framebuffer, 1000, 1000);

    custom_shader_.disable();

    GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
    GL_CHECK(glDeleteVertexArrays(1, &empty_vao));
}

unsigned int loadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(("../" + faces[i]).c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void Renderer::draw(const mat4& projection_matrix, const mat4& modelview_matrix, const std::string& draw_mode)
{

    // did we generate buffers already?
    if (!vertex_array_object_ || !background_array_object)
    {
        update_opengl_buffers();
    }

    // load shader?
    if (!phong_shader_.is_valid())
    {
        try
        {
            phong_shader_.source(phong_vshader, phong_fshader);
        }
        catch (const GLException& e)
        {
            std::cerr << e.what() << std::endl;
            exit(1);
        }
    }

    if (!custom_shader_.is_valid())
    {
        load_custom_shader();
    }

    if (!texture_shader_.is_valid())
    {
        load_texture_shader();
    }

    // load shader?
    if (!matcap_shader_.is_valid())
    {
        try
        {
            matcap_shader_.source(matcap_vshader, matcap_fshader);
        }
        catch (const GLException& e)
        {
            std::cerr << e.what() << std::endl;
            exit(1);
        }
    }

    // we need some texture, otherwise WebGL complains
    if (!texture_)
    {
        use_checkerboard_texture();
    }

    float skyboxVertices[] = {// positions
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

    std::vector<std::string> faces = {"right.jpg", "left.jpg", "top.jpg", "bottom.jpg", "front.jpg", "back.jpg"};
    unsigned int cubemap_texture = loadCubemap(faces);

    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, oldFBO));

    mat4 mv_matrix = modelview_matrix;
    mat4 mvp_matrix = projection_matrix * modelview_matrix;
    mat3 n_matrix = inverse(transpose(linear_part(mv_matrix)));

    GL_CHECK(glBindVertexArray(vertex_array_object_));

    static GLuint skyboxVAO = 0;
    static GLuint skyboxVBO = 0;
    if (!skyboxVAO)
    {
        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);
        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    }

    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    mat4 pro = pmp::perspective_matrix(90.0f, (float)wsize_ / (float)hsize_, 0.01f, 100.f);
    pro = pmp::mat4::identity();
    mat4 view = pmp::mat4::identity();

    texture_shader_.use();

    // std::cout << "Projection: \n" << pro << std::endl;
    // std::cout << "View: \n" << view << std::endl;

    texture_shader_.set_uniform("projection", pro);
    texture_shader_.set_uniform("view", view);

    // CreateCubeTexture();
    // // Draw the cubemap.
    // GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_framebuffer));
    // GL_CHECK(glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, g_depthbuffer));
    // for (int i = 0; i < 6; i++)
    // {
    //     drawFace(i);
    // }
    // GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
    // GL_CHECK(glBindVertexArray(0));

    // allow for transparent objects
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // GL_CHECK(glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE));
    GL_CHECK(glViewport(0, 0, wsize_, hsize_));
    // CODE from:
    // https://learnopengl.com/code_viewer_gh.php?code=src/4.advanced_opengl/6.1.cubemaps_skybox/cubemaps_skybox.cpphttps://learnopengl.com/code_viewer_gh.php?code=src/4.advanced_opengl/6.1.cubemaps_skybox/cubemaps_skybox.cpp

    GL_CHECK(glDepthMask(GL_FALSE));
    GL_CHECK(glBindVertexArray(skyboxVAO));
    GL_CHECK(glActiveTexture(GL_TEXTURE0));

    GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture));
    // GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, g_cubeTexture));

    GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 36));
    GL_CHECK(glBindVertexArray(0));

    GL_CHECK(glDepthMask(GL_TRUE));

    // if (mesh_.n_faces())
    // {
    //     GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, n_vertices_));
    // }

    // GL_CHECK(glDeleteVertexArrays(1, &empty_vao));

    // // Always check that our framebuffer is ok
    // if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    // {
    //     std::cerr << "Error: Framebuffer invalid" << std::endl;
    //     std::cerr << "This means you're missing something in the framebuffer! See: "
    //                  "https://learnopengl.com/Advanced-OpenGL/Framebuffers"
    //               << std::endl;
    //     std::cerr << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
    //     return;
    // }

    GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));

    // empty mesh?
    // if (mesh_.is_empty())
    //     return;

    // // setup matrices

    // // setup shader
    // phong_shader_.use();
    // phong_shader_.set_uniform("modelview_projection_matrix", mvp_matrix);
    // phong_shader_.set_uniform("modelview_matrix", mv_matrix);
    // phong_shader_.set_uniform("normal_matrix", n_matrix);
    // phong_shader_.set_uniform("point_size", point_size_);
    // phong_shader_.set_uniform("light1", vec3(1.0, 1.0, 1.0));
    // phong_shader_.set_uniform("light2", vec3(-1.0, 1.0, 1.0));
    // phong_shader_.set_uniform("front_color", front_color_);
    // phong_shader_.set_uniform("back_color", back_color_);
    // phong_shader_.set_uniform("ambient", ambient_);
    // phong_shader_.set_uniform("diffuse", diffuse_);
    // phong_shader_.set_uniform("specular", specular_);
    // phong_shader_.set_uniform("shininess", shininess_);
    // phong_shader_.set_uniform("alpha", alpha_);
    // phong_shader_.set_uniform("use_lighting", true);
    // phong_shader_.set_uniform("use_texture", false);
    // phong_shader_.set_uniform("use_srgb", false);
    // phong_shader_.set_uniform("show_texture_layout", false);
    // phong_shader_.set_uniform("use_vertex_color", has_vertex_colors_ && use_colors_);

    // GL_CHECK(glBindVertexArray(vertex_array_object_));
    // texture_shader_.use();

    // glActiveTexture(GL_TEXTURE0 + g_cubeTexUnit);
    // glBindTexture(GL_TEXTURE_CUBE_MAP, g_cubeTexture);

    // if (mesh_.n_faces())
    // {
    //     GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, n_vertices_));
    // }

    //     if (draw_mode == "Points")
    //     {
    // #ifndef __EMSCRIPTEN__
    //         glEnable(GL_PROGRAM_POINT_SIZE);
    // #endif
    //         GL_CHECK(glDrawArrays(GL_POINTS, 0, n_vertices_));
    //     }

    //     else if (draw_mode == "Hidden Line")
    //     {
    //         if (mesh_.n_faces())
    //         {
    //             // draw faces
    //             GL_CHECK(glDepthRange(0.01, 1.0));
    //             GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, n_vertices_));
    //             GL_CHECK(glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE));

    //             // overlay edges
    //             GL_CHECK(glDepthRange(0.0, 1.0));
    //             GL_CHECK(glDepthFunc(GL_LEQUAL));
    //             phong_shader_.set_uniform("front_color", vec3(0.1, 0.1, 0.1));
    //             phong_shader_.set_uniform("back_color", vec3(0.1, 0.1, 0.1));
    //             phong_shader_.set_uniform("use_lighting", false);
    //             phong_shader_.set_uniform("use_vertex_color", false);
    //             GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, edge_buffer_));
    //             GL_CHECK(glDrawElements(GL_LINES, n_edges_, GL_UNSIGNED_INT, nullptr));
    //             GL_CHECK(glDepthFunc(GL_LESS));
    //         }
    //     }
    //     else if (draw_mode == "Fractal Mode")
    //     {
    //         // SEE: https://stackoverflow.com/a/21652955
    //         GL_CHECK(glBindVertexArray(background_array_object));
    //         // https://stackoverflow.com/a/59739538
    //         custom_shader_.use();

    //         // set resolution
    //         custom_shader_.set_uniform("window_height", hsize_);
    //         custom_shader_.set_uniform("window_width", wsize_);
    //         custom_shader_.set_uniform("iTime", itime);
    //         custom_shader_.set_uniform("viewRotation", view_directions_[counter_]);
    //         vec3 origin = vec3(0, 0, 0);
    //         custom_shader_.set_uniform("origin", origin);
    //         // custom_shader_.set_uniform("camera_origin", camera_origin);
    //         // custom_shader_.set_uniform("camera_direction", camera_direction);

    //         GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 3));
    //         GL_CHECK(glBindVertexArray(vertex_array_object_));
    //     }
    //     else if (draw_mode == "Fractal Mode With Mesh")
    //     {
    //         // SEE: https://stackoverflow.com/a/21652955
    //         // GL_CHECK(glBindVertexArray(background_array_object));
    //         // // https://stackoverflow.com/a/59739538
    //         // custom_shader_.use();

    //         // // set resolution
    //         // custom_shader_.set_uniform("window_height", hsize_);
    //         // custom_shader_.set_uniform("window_width", wsize_);
    //         // custom_shader_.set_uniform("viewRotation", view_directions_[counter_]);
    //         // custom_shader_.set_uniform("origin", origin);

    //         // custom_shader_.set_uniform("iTime", itime);

    //         // // custom_shader_.set_uniform("camera_origin", camera_origin);
    //         // // custom_shader_.set_uniform("camera_direction", camera_direction);

    //         // GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 3));

    //         // setup shader
    //         phong_shader_.use();
    //         phong_shader_.set_uniform("modelview_projection_matrix", mvp_matrix);
    //         phong_shader_.set_uniform("modelview_matrix", mv_matrix);
    //         phong_shader_.set_uniform("normal_matrix", n_matrix);
    //         phong_shader_.set_uniform("point_size", point_size_);
    //         phong_shader_.set_uniform("light1", vec3(1.0, 1.0, 1.0));
    //         phong_shader_.set_uniform("light2", vec3(-1.0, 1.0, 1.0));
    //         phong_shader_.set_uniform("front_color", front_color_);
    //         phong_shader_.set_uniform("back_color", back_color_);
    //         phong_shader_.set_uniform("ambient", ambient_);
    //         phong_shader_.set_uniform("diffuse", diffuse_);
    //         phong_shader_.set_uniform("specular", specular_);
    //         phong_shader_.set_uniform("shininess", shininess_);
    //         phong_shader_.set_uniform("alpha", alpha_);
    //         phong_shader_.set_uniform("use_lighting", true);
    //         phong_shader_.set_uniform("use_texture", false);
    //         phong_shader_.set_uniform("use_srgb", false);
    //         phong_shader_.set_uniform("show_texture_layout", false);
    //         phong_shader_.set_uniform("use_vertex_color", has_vertex_colors_ && use_colors_);

    //         GL_CHECK(glBindVertexArray(vertex_array_object_));

    //         glActiveTexture(GL_TEXTURE0 + g_cubeTexUnit);
    //         glBindTexture(GL_TEXTURE_CUBE_MAP, g_cubeTexture);

    //         if (mesh_.n_faces())
    //         {
    //             GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, n_vertices_));
    //         }
    //     }

    //     else if (draw_mode == "Smooth Shading")
    //     {
    //         if (mesh_.n_faces())
    //         {
    //             GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, n_vertices_));
    //         }
    //     }

    //     else if (draw_mode == "Texture")
    //     {
    //         if (mesh_.n_faces())
    //         {
    //             if (texture_mode_ == TextureMode::MatCap)
    //             {
    //                 matcap_shader_.use();
    //                 matcap_shader_.set_uniform("modelview_projection_matrix", mvp_matrix);
    //                 matcap_shader_.set_uniform("normal_matrix", n_matrix);
    //                 matcap_shader_.set_uniform("alpha", alpha_);
    //                 GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture_));
    //                 GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, n_vertices_));
    //             }
    //             else
    //             {
    //                 phong_shader_.set_uniform("front_color", vec3(0.9, 0.9, 0.9));
    //                 phong_shader_.set_uniform("back_color", vec3(0.3, 0.3, 0.3));
    //                 phong_shader_.set_uniform("use_texture", true);
    //                 phong_shader_.set_uniform("use_vertex_color", false);
    //                 phong_shader_.set_uniform("use_srgb", use_srgb_);
    //                 GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture_));
    //                 GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, n_vertices_));
    //             }
    //         }
    //     }

    //     else if (draw_mode == "Texture Layout")
    //     {
    //         if (mesh_.n_faces() && has_texcoords_)
    //         {
    //             phong_shader_.set_uniform("show_texture_layout", true);
    //             phong_shader_.set_uniform("use_vertex_color", false);
    //             phong_shader_.set_uniform("use_lighting", false);

    //             // draw faces
    //             phong_shader_.set_uniform("front_color", vec3(0.8, 0.8, 0.8));
    //             phong_shader_.set_uniform("back_color", vec3(0.9, 0.0, 0.0));
    //             GL_CHECK(glDepthRange(0.01, 1.0));
    //             GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, n_vertices_));

    //             // overlay edges
    //             GL_CHECK(glDepthRange(0.0, 1.0));
    //             GL_CHECK(glDepthFunc(GL_LEQUAL));
    //             phong_shader_.set_uniform("front_color", vec3(0.1, 0.1, 0.1));
    //             phong_shader_.set_uniform("back_color", vec3(0.1, 0.1, 0.1));
    //             GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, edge_buffer_));
    //             GL_CHECK(glDrawElements(GL_LINES, n_edges_, GL_UNSIGNED_INT, nullptr));
    //             GL_CHECK(glDepthFunc(GL_LESS));
    //         }
    //     }

    //     // draw feature edges
    //     if (n_features_)
    //     {
    //         phong_shader_.set_uniform("front_color", vec3(0, 1, 0));
    //         phong_shader_.set_uniform("back_color", vec3(0, 1, 0));
    //         phong_shader_.set_uniform("use_vertex_color", false);
    //         phong_shader_.set_uniform("use_lighting", false);
    //         GL_CHECK(glDepthRange(0.0, 1.0));
    //         GL_CHECK(glDepthFunc(GL_LEQUAL));
    //         GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, feature_buffer_));
    //         GL_CHECK(glDrawElements(GL_LINES, n_features_, GL_UNSIGNED_INT, nullptr));
    //         GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    //         GL_CHECK(glDepthFunc(GL_LESS));
    //     }

    // disable transparency (doesn't work well with imgui)
    // GL_CHECK(glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE));

    GL_CHECK(glBindVertexArray(0));
    GL_CHECK(glCheckError());

    auto time_now = std::chrono::high_resolution_clock::now();

    /* Getting number of milliseconds as an integer. */
    auto ms_delta = std::chrono::duration_cast<std::chrono::milliseconds>(time_now - last_time);
    last_time = time_now;
    framerate = 1000.0 / ms_delta.count();
}

void Renderer::tesselate(const std::vector<vec3>& points, std::vector<ivec3>& triangles)
{
    const int n = points.size();

    triangles.clear();
    triangles.reserve(n - 2);

    // triangle? nothing to do
    if (n == 3)
    {
        triangles.emplace_back(0, 1, 2);
        return;
    }

    // quad? simply compare to two options
    else if (n == 4)
    {
        if (area(points[0], points[1], points[2]) + area(points[0], points[2], points[3])
            < area(points[0], points[1], points[3]) + area(points[1], points[2], points[3]))
        {
            triangles.emplace_back(0, 1, 2);
            triangles.emplace_back(0, 2, 3);
        }
        else
        {
            triangles.emplace_back(0, 1, 3);
            triangles.emplace_back(1, 2, 3);
        }
        return;
    }

    // n-gon with n>4? compute triangulation by dynamic programming
    init_triangulation(n);
    int i, j, m, k, imin;
    Scalar w, wmin;

    // initialize 2-gons
    for (i = 0; i < n - 1; ++i)
    {
        triangulation(i, i + 1) = Triangulation(0.0, -1);
    }

    // n-gons with n>2
    for (j = 2; j < n; ++j)
    {
        // for all n-gons [i,i+j]
        for (i = 0; i < n - j; ++i)
        {
            k = i + j;

            wmin = std::numeric_limits<Scalar>::max();
            imin = -1;

            // find best split i < m < i+j
            for (m = i + 1; m < k; ++m)
            {
                w = triangulation(i, m).area + area(points[i], points[m], points[k]) + triangulation(m, k).area;

                if (w < wmin)
                {
                    wmin = w;
                    imin = m;
                }
            }

            triangulation(i, k) = Triangulation(wmin, imin);
        }
    }

    // build triangles from triangulation table
    std::vector<ivec2> todo;
    todo.reserve(n);
    todo.emplace_back(0, n - 1);
    while (!todo.empty())
    {
        ivec2 tri = todo.back();
        todo.pop_back();
        int start = tri[0];
        int end = tri[1];
        if (end - start < 2)
            continue;
        int split = triangulation(start, end).split;

        triangles.emplace_back(start, split, end);

        todo.emplace_back(start, split);
        todo.emplace_back(split, end);
    }
}

} // namespace pmp
