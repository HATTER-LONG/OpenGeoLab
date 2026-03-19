set(command_sources
  src/BackendDispatcher.cpp
)

set(command_public_headers
  include/opengeolab/command/BackendDispatcher.hpp
  include/opengeolab/command/JsonProtocol.hpp
)

opengeolab_add_module(
  ogl_command
  ALIAS_NAME
    Command
  SOURCES
    ${command_sources}
  PUBLIC_HEADERS
    ${command_public_headers}
  PUBLIC_LINKS
    fmt::fmt
    nlohmann_json::nlohmann_json
    OpenGeoLab::Geometry
)

if(OPENGEOLAB_BUILD_TESTS)
  opengeolab_add_doctest_test(
    ogl_command_test
    SOURCES
      test/BackendDispatcherTest.cpp
    LINKS
      OpenGeoLab::Command
  )
endif()
