add_executable(beam)

target_sources(beam
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/application.hpp
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/application.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/beam.m.cpp
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

set_property(TARGET beam 
    PROPERTY 
        VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

