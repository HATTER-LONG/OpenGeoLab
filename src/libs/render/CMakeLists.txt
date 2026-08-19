# ---------------------------------------------------------------------------
# OpenGeoLab Render Library — Qt Rendering Hardware Interface backend
# ---------------------------------------------------------------------------

set(render_public_headers
    include/opengeolab/render/frame_state.hpp
    include/opengeolab/render/pick_mask.hpp
    include/opengeolab/render/pick_result.hpp
    include/opengeolab/render/render_pipeline.hpp
    include/opengeolab/render/render_scene_snapshot.hpp)

set(render_sources
    src/render_pipeline.cpp
    src/render_scene_snapshot.cpp
    src/pick_resolver.cpp
    src/label_anchor.cpp
    src/font/font_atlas.cpp)

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
    glm::glm
    PRIVATE_LINKS
    Qt6::Core
    Qt6::Gui
    Qt6::GuiPrivate
    nlohmann_json::nlohmann_json)

target_include_directories(opengeolab_render
                           PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

qt_add_shaders(
    opengeolab_render
    opengeolab_render_shaders
    PRECOMPILE
    PREFIX
    /opengeolab/render/shaders
    FILES
    resource/shaders/mesh.vert
    resource/shaders/mesh.frag
    resource/shaders/point.vert
    resource/shaders/point.frag
    resource/shaders/label.vert
    resource/shaders/label.frag
    resource/shaders/simple.vert
    resource/shaders/simple.frag)

# Copy MSDF atlas resources to build output for runtime loading.
set(RENDER_RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resource")
set(RENDER_RESOURCE_OUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/fonts")

file(GLOB FONT_RESOURCES "${RENDER_RESOURCE_DIR}/fonts/*")
if(FONT_RESOURCES)
    file(MAKE_DIRECTORY "${RENDER_RESOURCE_OUT}")
    foreach(res IN LISTS FONT_RESOURCES)
        get_filename_component(fname "${res}" NAME)
        configure_file("${res}" "${RENDER_RESOURCE_OUT}/${fname}" COPYONLY)
    endforeach()
endif()

if (OPENGEOLAB_BUILD_TESTS)
    opengeolab_add_doctest_test(
        opengeolab_render_test
        SOURCES
        test/pick_resolver_test.cpp
        test/render_pipeline_test.cpp
        test/render_scene_snapshot_test.cpp
        test/label_anchor_test.cpp
        test/font_atlas_test.cpp
        LINKS
        OpenGeoLab::Render)
    target_include_directories(opengeolab_render_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
endif ()
