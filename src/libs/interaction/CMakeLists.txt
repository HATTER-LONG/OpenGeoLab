set(interaction_sources
  src/InteractionRecorder.cpp
)

set(interaction_public_headers
  include/opengeolab/interaction/InteractionRecorder.hpp
)

opengeolab_add_module(
  ogl_interaction
  ALIAS_NAME
    Interaction
  SOURCES
    ${interaction_sources}
  PUBLIC_HEADERS
    ${interaction_public_headers}
  PUBLIC_LINKS
    OpenGeoLab::Render
    OpenGeoLab::Selection
    fmt::fmt
    nlohmann_json::nlohmann_json
)

if(OPENGEOLAB_BUILD_TESTS)
  opengeolab_add_doctest_test(
    ogl_interaction_test
    SOURCES
      test/InteractionRecorderTest.cpp
    LINKS
      OpenGeoLab::Interaction
  )
endif()
