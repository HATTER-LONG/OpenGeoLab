set(command_sources src/command_dispatcher.cpp src/module_dispatcher.cpp)

set(command_public_headers
    include/opengeolab/command/command_result.hpp
    include/opengeolab/command/action_interface.hpp
    include/opengeolab/command/module_service_interface.hpp
    include/opengeolab/command/module_dispatcher.hpp
    include/opengeolab/command/command_interface.hpp
    include/opengeolab/command/command_dispatcher.hpp)

opengeolab_add_module(
    ogl_command
    ALIAS_NAME
    Command
    SOURCES
    ${command_sources}
    PUBLIC_HEADERS
    ${command_public_headers}
    PUBLIC_LINKS
    Kangaroo::Kangaroo
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

  opengeolab_add_doctest_test(
      ogl_module_dispatcher_test
      SOURCES
      tests/module_dispatcher_test.cpp
      LINKS
      OpenGeoLab::Command
      Kangaroo::Kangaroo
      CTEST_NAME
      ogl_module_dispatcher_test)
endif()
