set(geometry_public_headers
    include/opengeolab/geometry/geometry_module.hpp
    include/opengeolab/geometry/create_box_action.hpp)
set(geometry_sources src/geometry_module.cpp src/create_box_action.cpp)

opengeolab_add_module(
    opengeolab_geometry
    ALIAS_NAME
    Geometry
    SOURCES
    ${geometry_sources}
    PUBLIC_HEADERS
    ${geometry_public_headers}
    PUBLIC_LINKS
    OpenGeoLab::Core)

if (OPENGEOLAB_BUILD_TESTS)
    opengeolab_add_doctest_test(
        opengeolab_geometry_test SOURCES test/geometry_module_test.cpp
        LINKS OpenGeoLab::Geometry)
endif ()
