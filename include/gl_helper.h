#pragma once

#include <GL/glu.h>
#include <GLFW/glfw3.h>
#include <iostream>

inline void check_open_gl_error(const char* stat, const char* fname, int line)
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
        check_open_gl_error(#stat, __FILE__, __LINE__);                                                                \
    } while (0)
