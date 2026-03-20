set(selection_sources
  src/SelectionService.cpp
)

set(selection_public_headers
  include/opengeolab/selection/SelectionService.hpp
)

opengeolab_add_module(
  ogl_selection
  ALIAS_NAME
    Selection
  SOURCES
    ${selection_sources}
  PUBLIC_HEADERS
    ${selection_public_headers}
  PUBLIC_LINKS
    OpenGeoLab::Render
    fmt::fmt
    nlohmann_json::nlohmann_json
)

if(OPENGEOLAB_BUILD_TESTS)
  opengeolab_add_doctest_test(
    ogl_selection_test
    SOURCES
      test/SelectionServiceTest.cpp
    LINKS
      OpenGeoLab::Selection
  )
endif()
