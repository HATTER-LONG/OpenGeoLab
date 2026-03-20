set(command_sources
    src/command_dispatcher.cpp
    src/bounding_box_command.cpp
    src/set_points_command.cpp
    src/get_stored_bbox_command.cpp)

set(command_public_headers
    include/opengeolab/command/command_interface.hpp
    include/opengeolab/command/command_dispatcher.hpp
    include/opengeolab/command/bounding_box_command.hpp
    include/opengeolab/command/set_points_command.hpp
    include/opengeolab/command/get_stored_bbox_command.hpp)

opengeolab_add_module(
    ogl_command
    ALIAS_NAME
    Command
    SOURCES
    ${command_sources}
    PUBLIC_HEADERS
    ${command_public_headers}
    PUBLIC_LINKS
    OpenGeoLab::Geometry
    nlohmann_json::nlohmann_json)
opengeolab_copy_runtime_dlls(ogl_command)

if(OPENGEOLAB_BUILD_TESTS)
  opengeolab_add_doctest_test(
      ogl_command_test
      SOURCES
      tests/command_dispatcher_test.cpp
      LINKS
      OpenGeoLab::Command
      CTEST_NAME
      ogl_command_test)
endif()
