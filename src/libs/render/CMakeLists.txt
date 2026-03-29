# ── libs/render ───────────────────────────────────────────────────────────────
# OpenGL rendering engine: Camera, TrackballController, ShaderProgram, GpuMesh,
# RenderScene, RenderEngine, ViewportItem (QQuickFramebufferObject).
# ─────────────────────────────────────────────────────────────────────────────

set(render_public_headers
    include/opengeolab/render/camera.hpp
    include/opengeolab/render/trackball_controller.hpp
    include/opengeolab/render/shader_program.hpp
    include/opengeolab/render/gpu_mesh.hpp
    include/opengeolab/render/render_scene.hpp
    include/opengeolab/render/render_engine.hpp
    include/opengeolab/render/viewport_item.hpp
    include/opengeolab/render/viewport_renderer.hpp)

set(render_sources
    src/camera.cpp
    src/trackball_controller.cpp
    src/shader_program.cpp
    src/gpu_mesh.cpp
    src/render_scene.cpp
    src/render_engine.cpp
    src/viewport_item.cpp
    src/viewport_renderer.cpp)

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
    Qt6::Quick
    Qt6::OpenGL
    glm::glm
    PRIVATE_LINKS
    glad_gl_core_46)

qt_add_resources(opengeolab_render "shaders"
    PREFIX "/"
    FILES
        shaders/phong.vert
        shaders/phong.frag
        shaders/edge.vert
        shaders/edge.frag)

if(OPENGEOLAB_BUILD_TESTS)
    opengeolab_add_doctest_test(
        opengeolab_camera_test
        SOURCES test/camera_test.cpp
        LINKS OpenGeoLab::Render)
endif()
