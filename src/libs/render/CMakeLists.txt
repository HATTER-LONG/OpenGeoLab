set(render_public_headers include/opengeolab/render/camera.hpp)

set(render_sources src/camera.cpp)

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
    glm::glm)

if (OPENGEOLAB_BUILD_TESTS)
    opengeolab_add_doctest_test(opengeolab_render_camera_test SOURCES
                                tests/camera_test.cpp LINKS OpenGeoLab::Render)
endif ()
