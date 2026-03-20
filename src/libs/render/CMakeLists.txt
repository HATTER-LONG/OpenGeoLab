set(render_sources
  src/RenderService.cpp
)

set(render_public_headers
  include/opengeolab/render/RenderService.hpp
)

opengeolab_add_module(
  ogl_render
  ALIAS_NAME
    Render
  SOURCES
    ${render_sources}
  PUBLIC_HEADERS
    ${render_public_headers}
  PUBLIC_LINKS
    fmt::fmt
    nlohmann_json::nlohmann_json
)

if(OPENGEOLAB_BUILD_TESTS)
  opengeolab_add_doctest_test(
    ogl_render_test
    SOURCES
      test/RenderServiceTest.cpp
    LINKS
      OpenGeoLab::Render
  )
endif()
