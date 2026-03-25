set(render_public_headers
    include/opengeolab/render/camera.hpp
    include/opengeolab/render/geometry_pass.hpp
    include/opengeolab/render/grid_pass.hpp
    include/opengeolab/render/render_context.hpp
    include/opengeolab/render/render_engine.hpp
    include/opengeolab/render/i_render_pass.hpp
    include/opengeolab/render/pass_manager.hpp
    include/opengeolab/render/shader_program.hpp
    include/opengeolab/render/vertex_array_object.hpp
    include/opengeolab/render/wireframe_pass.hpp)

set(render_sources
    src/camera.cpp
    src/geometry_pass.cpp
    src/grid_pass.cpp
    src/pass_manager.cpp
    src/render_engine.cpp
    src/shader_program.cpp
    src/vertex_array_object.cpp
    src/wireframe_pass.cpp)

opengeolab_add_module(
    opengeolab_render
    ALIAS_NAME
    Render
    SOURCES
    ${render_sources}
    PUBLIC_HEADERS
    ${render_public_headers}
    PUBLIC_LINKS
    OpenGeoLab::Scene
    glm::glm
    PRIVATE_LINKS
    glad)

if (OPENGEOLAB_BUILD_TESTS)
    opengeolab_add_doctest_test(opengeolab_render_camera_test SOURCES
                                tests/camera_test.cpp LINKS OpenGeoLab::Render)

    opengeolab_add_doctest_test(
        opengeolab_render_pass_manager_test SOURCES tests/pass_manager_test.cpp
        LINKS OpenGeoLab::Render)

    opengeolab_add_doctest_test(
        opengeolab_render_vertex_array_object_test SOURCES
        tests/vertex_array_object_test.cpp LINKS OpenGeoLab::Render)
endif ()
