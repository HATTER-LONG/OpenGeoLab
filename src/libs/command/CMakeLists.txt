set(command_public_headers include/opengeolab/command/command_dispatcher.hpp
                           include/opengeolab/command/module_registry.hpp)
set(command_sources src/command_dispatcher.cpp src/module_registry.cpp)

opengeolab_add_module(
    opengeolab_command
    ALIAS_NAME
    Command
    SOURCES
    ${command_sources}
    PUBLIC_HEADERS
    ${command_public_headers}
    PUBLIC_LINKS
    OpenGeoLab::Core
    OpenGeoLab::IO
    OpenGeoLab::Geometry
    OpenGeoLab::Scene)

if (OPENGEOLAB_BUILD_TESTS)
    opengeolab_add_doctest_test(
        opengeolab_command_test SOURCES test/command_dispatcher_test.cpp LINKS
        OpenGeoLab::Command)
endif ()
