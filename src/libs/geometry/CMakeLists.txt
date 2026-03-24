set(geometry_public_headers
    include/opengeolab/geometry/box_data.hpp
    include/opengeolab/geometry/create_box_action.hpp
    include/opengeolab/geometry/geometry_module.hpp)

set(geometry_sources src/create_box_action.cpp src/geometry_module.cpp)

opengeolab_add_module(
    opengeolab_geometry
    ALIAS_NAME
    Geometry
    SOURCES
    ${geometry_sources}
    PUBLIC_HEADERS
    ${geometry_public_headers}
    PUBLIC_LINKS
    OpenGeoLab::Base
    PRIVATE_LINKS
    nlohmann_json::nlohmann_json)

if (OPENGEOLAB_BUILD_TESTS)
    opengeolab_add_doctest_test(
        opengeolab_geometry_module_test SOURCES tests/geometry_module_test.cpp
        LINKS OpenGeoLab::Geometry nlohmann_json::nlohmann_json)
endif ()
