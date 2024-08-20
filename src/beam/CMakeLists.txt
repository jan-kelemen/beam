add_executable(beam)

target_sources(beam
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/application.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/free_camera_controller.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/perspective_camera.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/raytracer.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/scene.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/sphere.hpp
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/application.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/beam.m.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/free_camera_controller.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/perspective_camera.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/raytracer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/scene.cpp
)

target_include_directories(beam
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(beam
    PRIVATE
        Vulkan::Loader
    PRIVATE
        niku
    PRIVATE
        project-options
)
add_dependencies(beam shaders)

compile_shader(
    SHADER
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/raytracer.comp
    SPIRV
        ${CMAKE_CURRENT_BINARY_DIR}/raytracer.comp.spv
)

add_custom_target(shaders
    DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/raytracer.comp.spv
)

set_property(TARGET beam 
    PROPERTY 
        VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

