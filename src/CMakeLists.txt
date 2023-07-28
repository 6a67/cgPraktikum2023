### Add all source files and define library target
file(GLOB_RECURSE meshlife_SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE meshlife_HEADERS "${PROJECT_SOURCE_DIR}/include/*.h")

### Create target
add_library(meshlife ${meshlife_SOURCES} ${meshlife_HEADERS})
add_library(meshlife::meshlife ALIAS meshlife)

### Include own headers with public access
target_include_directories(meshlife PUBLIC "${PROJECT_SOURCE_DIR}/include")

### Find dependencies
# pmp
if (NOT TARGET pmp)
  add_subdirectory(${PROJECT_SOURCE_DIR}/extern/pmp-library extern/pmp-library EXCLUDE_FROM_ALL)
endif()
target_link_libraries(meshlife PUBLIC pmp pmp_vis)

set(GLFW_SOURCE_DIR "${PROJECT_SOURCE_DIR}/extern/pmp-library/external/glfw-3.3.8")
set(GLEW_SOURCE_DIR "${PROJECT_SOURCE_DIR}/extern/pmp-library/external/glew-2.2.0")
set(IMGUI_SOURCE_DIR "${PROJECT_SOURCE_DIR}/extern/pmp-library/external/imgui-1.89.5")
set(STBI_SOURCE_DIR "${PROJECT_SOURCE_DIR}/extern/pmp-library/external/stb_image-2.28")
set(STBI_WRITE_SOURCE_DIR "${PROJECT_SOURCE_DIR}/extern/pmp-library/external/stb_image_write-1.16")
target_include_directories(meshlife PUBLIC ${STBI_SOURCE_DIR} ${STBI_WRITE_SOURCE_DIR}
                           ${GLFW_SOURCE_DIR}/include ${GLFW_SOURCE_DIR}/deps
                           ${GLEW_SOURCE_DIR}/include ${IMGUI_SOURCE_DIR})

# Eigen
find_package(Eigen3 3.0 REQUIRED)
target_include_directories(meshlife PUBLIC ${EIGEN3_INCLUDE_DIR})

### Set properties
# properties
set_target_properties(meshlife PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)


# compile options
target_compile_options(meshlife PRIVATE "-Wall" "-Wextra" "-Wshadow" "-Wunused" "-Wunused-function")

# preprocessor defines
target_compile_definitions(meshlife PRIVATE "")
