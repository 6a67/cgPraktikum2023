#pragma once

#include <string>
#include <vector>
namespace meshlife

{
namespace stamps
{

enum Shapes
{
    s_none,
    s_orbium,
    s_smiley,
    s_debug,
    s_geminium,
    s_gyrorbium,
    s_velox,
    s_circle,
};
inline std::string shape_to_str(Shapes shape)
{
    switch (shape)
    {
    case Shapes::s_none:
        return "None";
        break;
    case Shapes::s_orbium:
        return "Orbium";
        break;
    case Shapes::s_smiley:
        return "Smiley";
        break;
    case Shapes::s_debug:
        return "Debug";
        break;
    case Shapes::s_geminium:
        return "Geminium";
        break;
    case Shapes::s_gyrorbium:
        return "Gyrorbium";
        break;
    case Shapes::s_velox:
        return "Velox";
        break;
    case Shapes::s_circle:
        return "Circle";
        break;
    }
    return "????";
}

inline std::vector<std::vector<float>> orbium
    = {{0, 0, 0, 0, 0, 0, 0.1, 0.14, 0.1, 0, 0, 0.03, 0.03, 0, 0, 0.3, 0, 0, 0, 0},
       {0, 0, 0, 0, 0, 0.08, 0.24, 0.3, 0.3, 0.18, 0.14, 0.15, 0.16, 0.15, 0.09, 0.2, 0, 0, 0, 0},
       {0, 0, 0, 0, 0, 0.15, 0.34, 0.44, 0.46, 0.38, 0.18, 0.14, 0.11, 0.13, 0.19, 0.18, 0.45, 0, 0, 0},
       {0, 0, 0, 0, 0.06, 0.13, 0.39, 0.5, 0.5, 0.37, 0.06, 0, 0, 0, 0.02, 0.16, 0.68, 0, 0, 0},
       {0, 0, 0, 0.11, 0.17, 0.17, 0.33, 0.4, 0.38, 0.28, 0.14, 0, 0, 0, 0, 0, 0.18, 0.42, 0, 0},
       {0, 0, 0.09, 0.18, 0.13, 0.06, 0.08, 0.26, 0.32, 0.32, 0.27, 0, 0, 0, 0, 0, 0, 0.82, 0, 0},
       {0.27, 0, 0.16, 0.12, 0, 0, 0, 0.25, 0.38, 0.44, 0.45, 0.34, 0, 0, 0, 0, 0, 0.22, 0.17, 0},
       {0, 0.07, 0.2, 0.02, 0, 0, 0, 0.31, 0.48, 0.57, 0.6, 0.57, 0, 0, 0, 0, 0, 0, 0.49, 0},
       {0, 0.59, 0.19, 0, 0, 0, 0, 0.2, 0.57, 0.69, 0.76, 0.76, 0.49, 0, 0, 0, 0, 0, 0.36, 0},
       {0, 0.58, 0.19, 0, 0, 0, 0, 0, 0.67, 0.83, 0.9, 0.92, 0.87, 0.12, 0, 0, 0, 0, 0.22, 0.07},
       {0, 0, 0.46, 0, 0, 0, 0, 0, 0.7, 0.93, 1, 1, 1, 0.61, 0, 0, 0, 0, 0.18, 0.11},
       {0, 0, 0.82, 0, 0, 0, 0, 0, 0.47, 1, 1, 0.98, 1, 0.96, 0.27, 0, 0, 0, 0.19, 0.1},
       {0, 0, 0.46, 0, 0, 0, 0, 0, 0.25, 1, 1, 0.84, 0.92, 0.97, 0.54, 0.14, 0.04, 0.1, 0.21, 0.05},
       {0, 0, 0, 0.4, 0, 0, 0, 0, 0.09, 0.8, 1, 0.82, 0.8, 0.85, 0.63, 0.31, 0.18, 0.19, 0.2, 0.01},
       {0, 0, 0, 0.36, 0.1, 0, 0, 0, 0.05, 0.54, 0.86, 0.79, 0.74, 0.72, 0.6, 0.39, 0.28, 0.24, 0.13, 0},
       {0, 0, 0, 0.01, 0.3, 0.07, 0, 0, 0.08, 0.36, 0.64, 0.7, 0.64, 0.6, 0.51, 0.39, 0.29, 0.19, 0.04, 0},
       {0, 0, 0, 0, 0.1, 0.24, 0.14, 0.1, 0.15, 0.29, 0.45, 0.53, 0.52, 0.46, 0.4, 0.31, 0.21, 0.08, 0, 0},
       {0, 0, 0, 0, 0, 0.08, 0.21, 0.21, 0.22, 0.29, 0.36, 0.39, 0.37, 0.33, 0.26, 0.18, 0.09, 0, 0, 0},
       {0, 0, 0, 0, 0, 0, 0.03, 0.13, 0.19, 0.22, 0.24, 0.24, 0.23, 0.18, 0.13, 0.05, 0, 0, 0, 0},
       {0, 0, 0, 0, 0, 0, 0, 0, 0.02, 0.06, 0.08, 0.09, 0.07, 0.05, 0.01, 0, 0, 0, 0, 0}};

inline std::vector<std::vector<float>> debug = {{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1},
                                                {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1},
                                                {1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
                                                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                                {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
                                                {0, 0, 0, 0, 0.5, 0, 0, 0, 0, 0},
                                                {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
                                                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                                {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
                                                {0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
                                                {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

inline std::vector<std::vector<float>> smiley = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 1, 0, 0, 0, 0, 1, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

inline std::vector<std::vector<float>> geminium = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    0,    0,    0,    0,    0,    0,    0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0.01, 0.02, 0.03, 0.04, 0.04, 0.04, 0.03, 0.02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0,   0,    0,    0,    0,    0,    0,    0,   0,    0,    0, 0, 0, 0, 0, 0, 0,    0,   0,
     0,   0,    0,    0,    0,    0,    0,    0,   0,    0,    0, 0, 0, 0, 0, 0, 0.04, 0.1, 0.16,
     0.2, 0.23, 0.25, 0.24, 0.21, 0.18, 0.14, 0.1, 0.07, 0.03, 0, 0, 0, 0, 0, 0, 0},
    {0,    0,    0,    0,    0,    0,    0,    0,   0,    0,    0,    0,    0, 0, 0,    0,    0,   0,    0,
     0,    0,    0,    0,    0,    0,    0,    0,   0,    0,    0,    0,    0, 0, 0.01, 0.09, 0.2, 0.33, 0.44,
     0.52, 0.56, 0.58, 0.55, 0.51, 0.44, 0.37, 0.3, 0.23, 0.16, 0.08, 0.01, 0, 0, 0,    0,    0},
    {0,    0,   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,    0,    0,    0,   0,
     0,    0,   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0.13, 0.29, 0.45, 0.6, 0.75,
     0.85, 0.9, 0.91, 0.88, 0.82, 0.74, 0.64, 0.55, 0.46, 0.36, 0.25, 0.12, 0.03, 0, 0,    0,    0},
    {0,   0,   0,   0,   0,   0,    0,    0,    0,    0,    0,    0,   0,    0,    0,    0,   0,    0,    0,
     0,   0,   0,   0,   0,   0,    0,    0,    0,    0,    0,    0,   0,    0.14, 0.38, 0.6, 0.78, 0.93, 1.0,
     1.0, 1.0, 1.0, 1.0, 1.0, 0.99, 0.89, 0.78, 0.67, 0.56, 0.44, 0.3, 0.15, 0.04, 0,    0,   0},
    {0,   0,   0,   0,   0,   0,   0,   0,    0,    0,    0,    0,    0,    0,    0,    0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,    0,    0,    0,    0,    0.08, 0.39, 0.74, 1.0, 1.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.98, 0.85, 0.74, 0.62, 0.47, 0.3,  0.14, 0.03, 0,   0},
    {0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0,    0,    0,    0,    0,    0,    0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0,    0,    0.32, 0.76, 1.0,  1.0,  1.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.88, 0.75, 0.61, 0.45, 0.27, 0.11, 0.01, 0},
    {0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0,    0,    0,    0,    0,    0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0.35, 0.83, 1.0,  1.0,  1.0,  1.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.88, 0.73, 0.57, 0.38, 0.19, 0.05, 0},
    {0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,    0,    0,    0,    0,    0,   0,   0,
     0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0.5, 1.0,  1.0,  1.0,  1.0,  1.0,  1.0, 1.0, 0.99,
     1.0, 1.0, 1.0, 1.0, 0.99, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.85, 0.67, 0.47, 0.27, 0.11, 0.01},
    {0,    0,    0,    0,    0,   0,    0,    0,   0,   0,    0,   0,    0,    0,    0,    0,    0,   0,    0,
     0,    0,    0,    0,    0,   0,    0,    0,   0,   0.55, 1.0, 1.0,  1.0,  1.0,  1.0,  1.0,  1.0, 0.93, 0.83,
     0.79, 0.84, 0.88, 0.89, 0.9, 0.93, 0.98, 1.0, 1.0, 1.0,  1.0, 0.98, 0.79, 0.57, 0.34, 0.15, 0.03},
    {0,    0,    0,   0,   0,    0,    0,    0,    0,    0,   0,   0,   0,    0,    0,    0,    0,   0,    0,
     0,    0,    0,   0,   0,    0,    0,    0,    0.47, 1.0, 1.0, 1.0, 1.0,  1.0,  1.0,  1.0,  0.9, 0.72, 0.54,
     0.44, 0.48, 0.6, 0.7, 0.76, 0.82, 0.91, 0.99, 1.0,  1.0, 1.0, 1.0, 0.91, 0.67, 0.41, 0.19, 0.05},
    {0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   0,   0,   0,   0,    0,    0,    0,    0,    0,
     0,    0,    0,    0,    0,    0,    0,    0.27, 0.99, 1.0, 1.0, 1.0, 1.0, 0.9,  0.71, 0.65, 0.55, 0.38, 0.2,
     0.14, 0.21, 0.36, 0.52, 0.64, 0.73, 0.84, 0.95, 1.0,  1.0, 1.0, 1.0, 1.0, 0.78, 0.49, 0.24, 0.07},
    {0, 0, 0,    0,    0,    0,    0,    0,    0,    0,   0,   0,   0,    0,    0,    0,    0,   0, 0,
     0, 0, 0,    0,    0,    0,    0.14, 0.63, 0.96, 1.0, 1.0, 1.0, 0.84, 0.17, 0,    0,    0,   0, 0,
     0, 0, 0.13, 0.35, 0.51, 0.64, 0.77, 0.91, 0.99, 1.0, 1.0, 1.0, 1.0,  0.88, 0.58, 0.29, 0.09},
    {0, 0, 0, 0,    0,    0,    0,    0,    0,    0,   0,   0,   0,    0,    0,    0,    0,  0, 0,
     0, 0, 0, 0,    0,    0.07, 0.38, 0.72, 0.95, 1.0, 1.0, 1.0, 0.22, 0,    0,    0,    0,  0, 0,
     0, 0, 0, 0.11, 0.33, 0.5,  0.67, 0.86, 0.99, 1.0, 1.0, 1.0, 1.0,  0.95, 0.64, 0.33, 0.1},
    {0, 0, 0, 0, 0,    0,    0,    0,    0,    0,   0,   0,    0,   0,    0,    0,    0,   0, 0,
     0, 0, 0, 0, 0.32, 0.49, 0.71, 0.93, 1.0,  1.0, 1.0, 0.56, 0,   0,    0,    0,    0,   0, 0,
     0, 0, 0, 0, 0.1,  0.31, 0.52, 0.79, 0.98, 1.0, 1.0, 1.0,  1.0, 0.98, 0.67, 0.35, 0.11},
    {0, 0, 0, 0,    0,   0,    0,    0,    0,    0,    0,   0,   0,   0,    0,    0,    0,   0, 0,
     0, 0, 0, 0.01, 0.6, 0.83, 0.98, 1.0,  1.0,  0.68, 0,   0,   0,   0,    0,    0,    0,   0, 0,
     0, 0, 0, 0,    0,   0.15, 0.38, 0.71, 0.97, 1.0,  1.0, 1.0, 1.0, 0.97, 0.67, 0.35, 0.11},
    {0, 0, 0, 0, 0,    0,    0,    0,    0,    0,   0,   0,   0,   0,    0,    0,    0,  0, 0,
     0, 0, 0, 0, 0.51, 0.96, 1.0,  1.0,  0.18, 0,   0,   0,   0,   0,    0,    0,    0,  0, 0,
     0, 0, 0, 0, 0,    0.09, 0.34, 0.68, 0.95, 1.0, 1.0, 1.0, 1.0, 0.91, 0.61, 0.32, 0.1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    0,    0,    0,    0,   0,   0,   0.13, 0.56, 0.99, 1.0,  1.0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.17, 0.45, 0.76, 0.96, 1.0, 1.0, 1.0, 1.0,  0.82, 0.52, 0.26, 0.07},
    {0, 0, 0,    0,   0,    0,    0,    0,    0,    0,   0,   0,   0,   0,    0,    0,    0,   0, 0,
     0, 0, 0.33, 0.7, 0.94, 1.0,  1.0,  0.44, 0,    0,   0,   0,   0,   0,    0,    0,    0,   0, 0,
     0, 0, 0,    0,   0,    0.33, 0.68, 0.91, 0.99, 1.0, 1.0, 1.0, 1.0, 0.71, 0.42, 0.19, 0.03},
    {0, 0,    0,    0,   0,   0,    0,    0,    0,    0,   0,   0,   0,    0,    0,    0,    0, 0, 0,
     0, 0.53, 0.89, 1.0, 1.0, 1.0,  0.8,  0.43, 0.04, 0,   0,   0,   0,    0,    0,    0,    0, 0, 0,
     0, 0,    0,    0,   0,   0.47, 0.86, 1.0,  1.0,  1.0, 1.0, 1.0, 0.95, 0.58, 0.32, 0.12, 0},
    {0, 0,    0,    0,   0,    0,    0,    0,    0,    0,   0,   0,   0,   0,    0,    0,    0, 0, 0,
     0, 0.77, 0.99, 1.0, 0.97, 0.58, 0.41, 0.33, 0.18, 0,   0,   0,   0,   0,    0,    0,    0, 0, 0,
     0, 0,    0,    0,   0,    0.54, 0.95, 1.0,  1.0,  1.0, 1.0, 1.0, 0.8, 0.44, 0.21, 0.06, 0},
    {0,    0,    0,   0,   0,    0,    0,    0,    0,    0,    0,   0,   0,    0,    0,    0,    0, 0, 0,
     0.39, 0.83, 1.0, 1.0, 0.55, 0.11, 0.05, 0.15, 0.22, 0.06, 0,   0,   0,    0,    0,    0,    0, 0, 0,
     0,    0,    0,   0,   0,    0.58, 0.99, 1.0,  1.0,  1.0,  1.0, 1.0, 0.59, 0.29, 0.11, 0.01, 0},
    {0,    0,    0,   0,   0,    0,    0,   0,    0,    0,    0,   0,    0,    0,    0,    0, 0.04, 0.55, 0.81,
     0.86, 0.97, 1.0, 1.0, 0.5,  0,    0,   0.01, 0.09, 0.03, 0,   0,    0,    0,    0,    0, 0,    0,    0,
     0,    0,    0,   0,   0.26, 0.78, 1.0, 1.0,  1.0,  1.0,  1.0, 0.66, 0.35, 0.13, 0.03, 0, 0},
    {0,   0,   0,    0,    0,    0,    0,   0,   0,   0,   0,    0,    0,    0, 0, 0, 0.33, 1.0, 1.0,
     1.0, 1.0, 1.0,  1.0,  0.93, 0.11, 0,   0,   0,   0,   0,    0,    0,    0, 0, 0, 0,    0,   0,
     0,   0,   0.23, 0.73, 0.95, 1.0,  1.0, 1.0, 1.0, 1.0, 0.62, 0.35, 0.12, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0,   0,   0.51, 1.0, 1.0, 1.0,  1.0,  1.0,  1.0, 1.0, 0.72, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1.0, 1.0, 1.0, 1.0, 1.0,  1.0, 1.0, 0.56, 0.25, 0.09, 0,   0,   0,    0, 0},
    {0,    0,    0,    0,   0,   0,   0,    0,   0,    0,    0, 0, 0, 0, 0.12, 0.38, 1.0, 1.0, 1.0,
     0.66, 0.08, 0.55, 1.0, 1.0, 1.0, 0.03, 0,   0,    0,    0, 0, 0, 0, 0,    0,    0,   0,   0,
     0,    0.35, 1.0,  1.0, 1.0, 1.0, 1.0,  1.0, 0.67, 0.12, 0, 0, 0, 0, 0,    0,    0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0.6, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,  0.49, 0, 0, 0.87, 1.0, 0.88, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.7, 0.07, 0,    0, 0, 0,    0,   0,    0, 0},
    {0,   0,   0,   0,    0,    0,    0,    0, 0, 0.04, 0.21, 0.48, 1.0, 1.0, 1.0, 1.0,  1.0,  1.0,  1.0,
     1.0, 0,   0,   0.04, 0.42, 0.26, 0,    0, 0, 0,    0,    0,    0,   0,   0,   0.12, 0.21, 0.34, 0.58,
     1.0, 1.0, 1.0, 0.99, 0.97, 0.99, 0.46, 0, 0, 0,    0,    0,    0,   0,   0,   0,    0},
    {0,   0,    0,   0,    0,    0,    0, 0, 0, 0.5, 1.0, 1.0,  1.0, 1.0, 0.96, 0,    0.31, 1.0, 1.0,
     1.0, 0.53, 0,   0,    0,    0,    0, 0, 0, 0,   0.2, 0.21, 0,   0,   0,    0.27, 1.0,  1.0, 1.0,
     1.0, 1.0,  1.0, 0.87, 0.52, 0.01, 0, 0, 0, 0,   0,   0,    0,   0,   0,    0,    0},
    {0,   0,    0,   0,    0, 0, 0, 0, 0.84, 1.0,  1.0,  1.0,  1.0,  1.0, 0, 0,    0,   0.83, 1.0,
     1.0, 0.52, 0,   0,    0, 0, 0, 0, 0,    0.26, 0.82, 0.59, 0.02, 0,   0, 0.46, 1.0, 1.0,  1.0,
     1.0, 1.0,  0.9, 0.55, 0, 0, 0, 0, 0,    0,    0,    0,    0,    0,   0, 0,    0},
    {0,    0,    0,    0, 0, 0, 0, 0.39, 0.99, 1.0,  1.0, 1.0, 1.0,  0.78, 0.04, 0,   0,   0,    0.93,
     0.92, 0,    0,    0, 0, 0, 0, 0,    0,    0.69, 1.0, 1.0, 0.36, 0,    0,    1.0, 1.0, 0.65, 0.66,
     0.97, 0.87, 0.54, 0, 0, 0, 0, 0,    0,    0,    0,   0,   0,    0,    0,    0,   0},
    {0,    0,    0, 0, 0.55, 0.75, 0.59, 0.74, 1.0, 1.0,  0,   0,   0.75, 0.71, 0.18, 0,   0,   0,    0,
     0,    0,    0, 0, 0,    0,    0.29, 0,    0,   0.45, 1.0, 1.0, 1.0,  1.0,  1.0,  1.0, 1.0, 0.47, 0.39,
     0.71, 0.25, 0, 0, 0,    0,    0,    0,    0,   0,    0,   0,   0,    0,    0,    0,   0},
    {0, 0, 0, 0, 0.69, 0.81, 0.8, 0.92, 1.0, 0.13, 0,   0,   0.13, 0.94, 0.58, 0,   0,   0,    0,
     0, 0, 0, 0, 0,    1.0,  1.0, 0.34, 0,   0.04, 1.0, 1.0, 1.0,  1.0,  1.0,  1.0, 1.0, 0.24, 0,
     0, 0, 0, 0, 0,    0,    0,   0,    0,   0,    0,   0,   0,    0,    0,    0,   0},
    {0, 0, 0, 0, 0.63, 0.85, 0.9, 0.98, 1.0, 0.09, 0,   0,   0.02, 1.0, 0.64, 0,   0,    0, 0,
     0, 0, 0, 0, 0.59, 1.0,  1.0, 0.84, 0,   0,    1.0, 1.0, 1.0,  1.0, 1.0,  1.0, 0.64, 0, 0,
     0, 0, 0, 0, 0,    0,    0,   0,    0,   0,    0,   0,   0,    0,   0,    0,   0},
    {0, 0, 0, 0, 0.64, 0.65, 0.67, 1.0, 1.0,  0.21, 0.01, 0,   0.04, 0.02, 0,   0,    0, 0, 0,
     0, 0, 0, 0, 0.69, 1.0,  1.0,  1.0, 0.29, 0.37, 1.0,  1.0, 0.6,  0.63, 1.0, 0.84, 0, 0, 0,
     0, 0, 0, 0, 0,    0,    0,    0,   0,    0,    0,    0,   0,    0,    0,   0,    0},
    {0, 0,    0, 0, 0.44, 0.73, 0.73, 0.85, 1.0, 0.97, 0.23, 0.05, 0,    0,    0,    0, 0, 0, 0,
     0, 0.06, 0, 0, 0,    0.97, 1.0,  1.0,  1.0, 1.0,  1.0,  1.0,  0.33, 0.24, 0.67, 0, 0, 0, 0,
     0, 0,    0, 0, 0,    0,    0,    0,    0,   0,    0,    0,    0,    0,    0,    0, 0},
    {0,    0,    0, 0.12, 0.55, 0.9,  0.9, 1.0, 1.0, 1.0, 0.43, 0.04, 0, 0, 0, 0, 0, 0, 0,
     0.31, 0.54, 0, 0,    0,    0.88, 1.0, 1.0, 1.0, 1.0, 1.0,  1.0,  0, 0, 0, 0, 0, 0, 0,
     0,    0,    0, 0,    0,    0,    0,   0,   0,   0,   0,    0,    0, 0, 0, 0, 0},
    {0,   0,    0,    0.29, 0.71, 1.0,  1.0, 1.0, 1.0, 0.79, 0.28, 0,    0, 0, 0, 0, 0, 0, 0,
     0.4, 0.77, 0.54, 0,    0,    0.87, 1.0, 1.0, 1.0, 1.0,  1.0,  0.31, 0, 0, 0, 0, 0, 0, 0,
     0,   0,    0,    0,    0,    0,    0,   0,   0,   0,    0,    0,    0, 0, 0, 0, 0},
    {0,   0.16, 0.27, 0.41, 0.72, 0.99, 1.0, 1.0, 0.82, 0.42, 0.09, 0, 0, 0, 0, 0, 0, 0, 0,
     0.1, 0.55, 0.58, 0.58, 0.77, 0.99, 1.0, 1.0, 1.0,  1.0,  0.63, 0, 0, 0, 0, 0, 0, 0, 0,
     0,   0,    0,    0,    0,    0,    0,   0,   0,    0,    0,    0, 0, 0, 0, 0, 0},
    {0.31, 0.48, 0.45, 0.46, 0.63, 0.88, 1.0, 0.83, 0.59, 0.28, 0.06, 0,    0, 0, 0, 0, 0, 0, 0,
     0,    0.32, 0.7,  0.95, 1.0,  1.0,  1.0, 1.0,  0.7,  0.58, 0.12, 0.04, 0, 0, 0, 0, 0, 0, 0,
     0,    0,    0,    0,    0,    0,    0,   0,    0,    0,    0,    0,    0, 0, 0, 0, 0},
    {0.23, 0.54, 0.53, 0.48, 0.57, 0.59, 0.65, 0.63, 0.55, 0.35, 0.13, 0.03, 0.02, 0.09, 0.74, 1.0, 0.09, 0, 0,
     0,    0.32, 0.86, 1.0,  1.0,  1.0,  1.0,  0.57, 0.44, 0.31, 0.16, 0.01, 0,    0,    0,    0,   0,    0, 0,
     0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   0},
    {0, 0.31, 0.45, 0.31, 0.18, 0.28, 0.39, 0.47, 0.54, 0.5, 0.35, 0.2, 0.16, 0.28, 0.75, 1.0, 0.42, 0.01, 0,
     0, 0.6,  1.0,  1.0,  1.0,  1.0,  0.51, 0.29, 0.09, 0,   0,    0,   0,    0,    0,    0,   0,    0,    0,
     0, 0,    0,    0,    0,    0,    0,    0,    0,    0,   0,    0,   0,    0,    0,    0,   0},
    {0,    0,   0,   0,   0,   0.14, 0.3,  0.4, 0.54, 0.71, 0.74, 0.65, 0.49, 0.35, 0.27, 0.47, 0.6, 0.6, 0.72,
     0.98, 1.0, 1.0, 1.0, 1.0, 0.65, 0.33, 0,   0,    0,    0,    0,    0,    0,    0,    0,    0,   0,   0,
     0,    0,   0,   0,   0,   0,    0,    0,   0,    0,    0,    0,    0,    0,    0,    0,    0},
    {0,   0,   0,   0,   0,   0.06, 0.33, 0.53, 0.69, 0.94, 0.99, 1.0, 0.84, 0.41, 0.16, 0.15, 0.96, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0, 1.0, 0.73, 0.13, 0,    0,    0,    0,    0,   0,    0,    0,    0,    0,    0,   0,
     0,   0,   0,   0,   0,   0,    0,    0,    0,    0,    0,    0,   0,    0,    0,    0,    0},
    {0,   0,   0,   0,   0,    0,    0.42, 0.86, 0.98, 0.98, 0.99, 1.0, 0.94, 0.63, 0.32, 0.62, 1.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0, 0.65, 0.23, 0,    0,    0,    0,    0,    0,   0,    0,    0,    0,    0,   0,   0,
     0,   0,   0,   0,   0,    0,    0,    0,    0,    0,    0,    0,   0,    0,    0,    0,    0},
    {0,   0,    0,    0, 0, 0.07, 0.62, 0.95, 1.0, 1.0, 0.99, 0.98, 0.99, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
     1.0, 0.98, 0.14, 0, 0, 0,    0,    0,    0,   0,   0,    0,    0,    0,   0,   0,   0,   0,   0,
     0,   0,    0,    0, 0, 0,    0,    0,    0,   0,   0,    0,    0,    0,   0,   0,   0},
    {0,    0,    0, 0, 0, 0.03, 0.46, 0.89, 1.0, 1.0, 0.97, 0.83, 0.75, 0.81, 0.94, 1.0, 1.0, 1.0, 1.0,
     0.99, 0.03, 0, 0, 0, 0,    0,    0,    0,   0,   0,    0,    0,    0,    0,    0,   0,   0,   0,
     0,    0,    0, 0, 0, 0,    0,    0,    0,   0,   0,    0,    0,    0,    0,    0,   0},
    {0,    0, 0, 0, 0, 0, 0.14, 0.57, 0.88, 0.93, 0.81, 0.58, 0.45, 0.48, 0.64, 0.86, 0.97, 0.99, 0.99,
     0.42, 0, 0, 0, 0, 0, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     0,    0, 0, 0, 0, 0, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0.23, 0.45, 0.47, 0.39, 0.29, 0.19, 0.2, 0.46, 0.28, 0.03, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0,    0,    0,    0,    0,    0,    0,   0,    0,    0,    0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.08, 0.22, 0.24, 0.15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    0,    0,    0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.07, 0.22, 0.14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    0,    0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

inline std::vector<std::vector<float>> gyrorbium = {
    {0, 0, 0, 0, 0, 0, 0, 0.19, 0.6, 0.7, 0.66, 0.57, 0.47, 0.39, 0.35, 0.34, 0.36, 0.29, 0.07},
    {0, 0, 0, 0, 0, 0.44, 0.84, 0.7, 0.57, 0.66, 0.86, 0.92, 0.8, 0.7, 0.67, 0.71, 0.75, 0.78, 0.73, 0.45},
    {0, 0, 0, 0, 0.43, 0.11, 0.26, 0.24, 0.19, 0.18, 0.2, 0.55, 0.95, 0.95, 0.84, 0.77, 0.65, 0.48, 0.44, 0.62, 0.61},
    {0, 0, 0, 0.24, 0, 0.18, 0.26, 0.15, 0.08, 0.07, 0.08, 0.14, 0.73, 1, 0.91, 0.61, 0.21, 0, 0, 0, 0.33, 0.53},
    {0, 0, 0.1, 0, 0, 0.22, 0.21, 0.04, 0, 0, 0, 0, 0.37, 0.84, 0.72, 0.33, 0, 0, 0, 0, 0, 0.37, 0.21},
    {0, 0, 0.07, 0, 0, 0.24, 0.15, 0, 0, 0, 0, 0, 0.25, 0.63, 0.54, 0.25, 0, 0, 0, 0, 0, 0, 0.51},
    {0, 0, 0.02, 0, 0, 0.24, 0.11, 0, 0, 0, 0, 0, 0.28, 0.52, 0.45, 0.23, 0, 0, 0, 0, 0, 0, 0.47, 0.01},
    {0, 0, 0.01, 0.11, 0, 0.2, 0.11, 0, 0, 0, 0, 0.13, 0.32, 0.56, 0.49, 0.31, 0.07, 0, 0, 0, 0, 0, 0.42, 0.07},
    {0, 0, 0.02, 0.31, 0.24, 0.15, 0.14, 0, 0, 0, 0, 0.12, 0.29, 0.54, 0.66, 0.69, 0.21, 0, 0, 0, 0, 0, 0.47, 0.09},
    {0,    0,    0.11, 0.36, 0.49, 0.33, 0.22, 0, 0, 0,    0,    0.11,
     0.27, 0.55, 0.72, 0.77, 0.77, 0.05, 0,    0, 0, 0.05, 0.59, 0.05},
    {0, 0,    0.19, 0.34, 0.31, 0.23, 0.24, 0,    0,   0,    0,    0,
     0, 0.24, 0.83, 0.93, 0.96, 0.47, 0.19, 0.04, 0.1, 0.44, 0.59, 0.01},
    {0, 0.04, 0, 0, 0, 0, 0.21, 0.03, 0, 0, 0, 0, 0, 0.1, 0.89, 1, 1, 0.9, 0.73, 0.61, 0.61, 0.69, 0.39},
    {0, 0, 0, 0, 0, 0, 0.16, 0.13, 0, 0, 0, 0, 0, 0.21, 0.67, 1, 1, 0.79, 0.78, 0.79, 0.74, 0.57, 0.17},
    {0.06, 0, 0, 0, 0, 0, 0.09, 0.22, 0.01, 0, 0, 0, 0.06, 0.32, 0.46, 0.93, 0.99, 0.59, 0.55, 0.56, 0.51, 0.32, 0.07},
    {0.07, 0, 0, 0, 0, 0, 0, 0.26, 0.15, 0.02, 0, 0.03, 0.18, 0.38, 0.54, 1, 0.99, 0.36, 0.27, 0.28, 0.27, 0.18, 0.03},
    {0.03, 0,    0,    0, 0,    0,    0,    0.15, 0.36, 0.28, 0.21, 0.25,
     0.4,  0.59, 0.92, 1, 0.45, 0.13, 0.11, 0.18, 0.19, 0.13, 0.02},
    {0, 0, 0, 0, 0, 0, 0, 0, 0.09, 0.38, 0.57, 0.71, 0.83, 0.95, 0.96, 0.48, 0.1, 0.1, 0.15, 0.18, 0.17, 0.09},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.06, 0.15, 0.13, 0.07, 0.08, 0.1, 0.12, 0.15, 0.16, 0.12, 0.04},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.01, 0.04, 0.07, 0.1, 0.12, 0.1, 0.04},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.02, 0.03, 0.03}};

inline std::vector<std::vector<float>> velox
    = {
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.04, 0.09, 0.07, 0.01},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.17, 0.52, 0.72, 0.84, 0.93, 0.98, 1, 0.89, 0.71, 0.47, 0.2, 0.01},
            {0, 0, 0, 0, 0, 0, 0, 0, 0.25, 0.41, 0.56, 0.71, 0.84, 0.95, 1, 1, 1, 1, 1, 1, 0.88, 0.55, 0.22, 0.01},
            {0, 0, 0, 0, 0, 0, 0, 0.18, 0.35, 0.52, 0.67, 0.81, 0.92, 1, 1, 1, 1, 1, 1, 1, 1, 0.98, 0.71, 0.37, 0.07},
            {0, 0, 0, 0, 0, 0, 0.07, 0.25, 0.43, 0.59, 0.74, 0.87, 0.97, 1, 1, 1, 1, 0.96, 0.94, 0.96, 0.99, 0.97, 0.89, 0.69, 0.39, 0.1},
            {0, 0, 0, 0, 0, 0, 0.13, 0.3, 0.47, 0.64, 0.79, 0.92, 1, 0.91, 0.75, 0.63, 0.63, 0.69, 0.8, 0.89, 0.9, 0.89, 0.84, 0.75, 0.59, 0.33, 0.07},
            {0, 0, 0, 0, 0, 0, 0.14, 0.32, 0.51, 0.63, 0.61, 0.49, 0.33, 0.14, 0.01, 0, 0.05, 0.25, 0.5, 0.72, 0.9, 0.98, 0.89, 0.76, 0.63, 0.45, 0.21, 0.02},
            {0, 0, 0, 0, 0, 0, 0.15, 0.28, 0.21, 0.18, 0.31, 0.29, 0.09, 0, 0, 0, 0, 0.06, 0.21, 0.43, 0.72, 0.93, 1, 0.88, 0.67, 0.48, 0.28, 0.08},
            {0, 0, 0, 0, 0, 0, 0, 0, 0.03, 0.21, 0.27, 0.12, 0, 0, 0, 0, 0, 0.12, 0.28, 0.44, 0.55, 0.76, 0.98, 1, 0.8, 0.52, 0.29, 0.1},
            {0, 0, 0, 0, 0, 0, 0, 0, 0.03, 0.2, 0.23, 0.03, 0, 0, 0, 0, 0.03, 0.24, 0.42, 0.57, 0.69, 0.77, 0.86, 1, 0.93, 0.63, 0.32, 0.1},
            {0, 0, 0, 0, 0, 0, 0, 0, 0.02, 0.21, 0.21, 0, 0, 0, 0, 0, 0.04, 0.29, 0.52, 0.71, 0.84, 0.92, 0.96, 0.91, 0.93, 0.75, 0.38, 0.1},
            {0, 0, 0, 0, 0, 0, 0, 0, 0.04, 0.27, 0.15, 0, 0, 0, 0, 0, 0, 0.29, 0.57, 0.8, 0.96, 1, 1, 0.98, 0.87, 0.78, 0.47, 0.11},
            {0, 0, 0, 0, 0, 0, 0, 0, 0.11, 0.33, 0.17, 0, 0, 0, 0, 0, 0.01, 0.27, 0.57, 0.86, 1, 1, 1, 1, 0.9, 0.75, 0.53, 0.15},
            {0, 0, 0, 0, 0, 0, 0, 0, 0.22, 0.39, 0.34, 0.15, 0, 0, 0, 0, 0.22, 0.43, 0.66, 0.91, 1, 1, 1, 1, 0.94, 0.74, 0.53, 0.18},
            {0, 0, 0, 0, 0, 0, 0, 0, 0.28, 0.43, 0.54, 0.45, 0.11, 0.02, 0.22, 0.42, 0.61, 0.77, 0.9, 0.99, 1, 1, 0.88, 0.93, 0.9, 0.75, 0.5, 0.18},
            {0, 0, 0, 0, 0, 0, 0, 0, 0.26, 0.48, 0.81, 0.53, 0, 0, 0, 0.05, 0.7, 0.95, 1, 1, 1, 1, 0.7, 0.66, 0.73, 0.7, 0.46, 0.14},
            {0, 0, 0, 0, 0, 0, 0, 0.08, 0.29, 0.74, 1, 0.13, 0, 0, 0, 0, 0, 0.84, 1, 1, 1, 1, 0.56, 0.42, 0.5, 0.58, 0.39, 0.08},
            {0, 0, 0, 0, 0, 0, 0, 0.21, 0.49, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0.43, 0.22, 0.27, 0.39, 0.28, 0.02},
            {0, 0, 0.04, 0.04, 0, 0, 0.12, 0.42, 0.76, 1, 1, 0, 0, 0.85, 1, 0, 0, 0, 1, 1, 1, 0.97, 0.27, 0.08, 0.08, 0.2, 0.15},
            {0, 0, 0.14, 0.16, 0.05, 0.11, 0.34, 0.64, 0.92, 1, 1, 0, 0, 0.96, 1, 0.15, 0, 0, 1, 1, 1, 0.75, 0.12, 0, 0, 0.05, 0.05},
            {0, 0, 0.25, 0.33, 0.22, 0.3, 0.57, 0.93, 0.98, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0.83, 0.45, 0.01},
            {0, 0.02, 0.35, 0.51, 0.43, 0.49, 0.76, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0.12, 1, 0.83, 0.57, 0.06},
            {0, 0.07, 0.4, 0.65, 0.63, 0.66, 0.9, 1, 1, 1, 1, 0.66, 0, 0, 0, 0, 0, 0.32, 0.82, 0.56, 0.31},
            {0, 0.12, 0.43, 0.73, 0.82, 0.85, 1, 1, 1, 1, 1, 0.87, 0.47, 0, 0, 0, 0.01, 0.38, 0.56, 0.42, 0.13},
            {0, 0.16, 0.45, 0.74, 0.96, 1, 1, 1, 1, 0.99, 0.93, 0.76, 0.5, 0.18, 0, 0, 0, 0.18, 0.38, 0.35, 0.03},
            {0, 0.18, 0.47, 0.71, 0.99, 1, 1, 1, 1, 0.91, 0.72, 0.5, 0.25, 0, 0, 0, 0, 0.02, 0.26, 0.3},
            {0, 0.18, 0.48, 0.69, 0.89, 1, 1, 1, 0.98, 0.83, 0.6, 0.35, 0.09, 0, 0, 0, 0, 0, 0.23, 0.25},
            {0, 0.17, 0.45, 0.68, 0.81, 1, 1, 1, 0.94, 0.76, 0.55, 0.31, 0.06, 0, 0, 0, 0, 0.01, 0.24, 0.22, 0.01},
            {0.01, 0.15, 0.4, 0.65, 0.79, 0.84, 0.98, 0.96, 0.87, 0.7, 0.5, 0.28, 0.06, 0, 0, 0, 0, 0.05, 0.25, 0.19},
            {0, 0.14, 0.36, 0.59, 0.77, 0.83, 0.8, 0.82, 0.78, 0.64, 0.46, 0.26, 0.06, 0, 0, 0, 0, 0.15, 0.29, 0.16},
            {0, 0.1, 0.32, 0.54, 0.72, 0.83, 0.82, 0.72, 0.6, 0.53, 0.4, 0.22, 0.05, 0, 0, 0, 0.09, 0.29, 0.28, 0.12},
            {0, 0.05, 0.26, 0.48, 0.68, 0.82, 0.86, 0.78, 0.64, 0.46, 0.27, 0.11, 0, 0, 0, 0.11, 0.3, 0.35, 0.29, 0.26, 0.25, 0.28, 0.1},
            {0, 0, 0.13, 0.39, 0.63, 0.8, 0.89, 0.87, 0.77, 0.64, 0.48, 0.32, 0.21, 0.21, 0.33, 0.51, 0.61, 0.65, 0.66, 0.6, 0.43, 0.26, 0.08},
            {0, 0, 0, 0.2, 0.49, 0.75, 0.94, 1, 1, 0.94, 0.87, 0.79, 0.77, 0.83, 0.97, 1, 0.96, 0.86, 0.71, 0.55, 0.39, 0.22, 0.04},
            {0, 0, 0, 0, 0.2, 0.52, 0.83, 1, 1, 1, 1, 1, 1, 1, 1, 0.99, 0.91, 0.78, 0.64, 0.48, 0.33, 0.15},
            {0, 0, 0, 0, 0, 0.12, 0.42, 0.77, 1, 1, 1, 1, 1, 1, 1, 0.93, 0.82, 0.69, 0.54, 0.39, 0.24, 0.07},
            {0, 0, 0, 0, 0, 0, 0, 0.17, 0.47, 0.75, 0.99, 1, 1, 0.98, 0.93, 0.84, 0.72, 0.58, 0.4, 0.03},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0.01, 0.15, 0.27, 0.34, 0.38, 0.33, 0.18}
        };

} // namespace stamps
} // namespace meshlife
