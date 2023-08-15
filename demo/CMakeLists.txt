add_executable(meshlife_demo meshlife_demo.cpp)
target_link_libraries(meshlife_demo meshlife)

add_custom_command(TARGET meshlife_demo POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/src/shaders $<TARGET_FILE_DIR:meshlife_demo>/shaders)
