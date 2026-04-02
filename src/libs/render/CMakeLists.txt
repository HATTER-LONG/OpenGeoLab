# ---------------------------------------------------------------------------
# OpenGeoLab Render Library — pure OpenGL, no Qt dependency
# ---------------------------------------------------------------------------

set(render_public_headers
    include/opengeolab/render/frame_state.hpp
    include/opengeolab/render/pick_mask.hpp
    include/opengeolab/render/pick_result.hpp
    include/opengeolab/render/render_pipeline.hpp
    include/opengeolab/render/batch_utils.hpp)

set(render_sources
    src/render_pipeline.cpp
    src/batch_utils.cpp
    src/pick_resolver.cpp
    src/core/shader_program.cpp
    src/core/gpu_buffer_manager.cpp
    src/core/thick_line_renderer.cpp
    src/core/pick_fbo.cpp
    src/pass/opaque_pass.cpp
    src/pass/wireframe_pass.cpp
    src/pass/highlight_pass.cpp
    src/pass/selection_pass.cpp)

opengeolab_add_module(
    opengeolab_render
    ALIAS_NAME
    Render
    SOURCES
    ${render_sources}
    PUBLIC_HEADERS
    ${render_public_headers}
    PUBLIC_LINKS
    OpenGeoLab::Core
    OpenGeoLab::Scene
    glad::glad
    glm::glm)

target_include_directories(opengeolab_render
                           PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

if (OPENGEOLAB_BUILD_TESTS)
    opengeolab_add_doctest_test(
        opengeolab_render_test
        SOURCES
        test/pick_resolver_test.cpp
        test/batch_utils_test.cpp
        test/render_pass_base_test.cpp
        test/selection_pass_test.cpp
        test/render_pipeline_test.cpp
        LINKS
        OpenGeoLab::Render)
    target_include_directories(opengeolab_render_test
                               PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
endif ()
