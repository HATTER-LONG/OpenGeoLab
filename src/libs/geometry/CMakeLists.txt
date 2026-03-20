set(geometry_sources src/bounding_box_calculator.cpp)

set(geometry_public_headers
    include/opengeolab/geometry/bounding_box.hpp
    include/opengeolab/geometry/bounding_box_calculator.hpp)

opengeolab_add_module(
    ogl_geometry
    ALIAS_NAME
    Geometry
    SOURCES
    ${geometry_sources}
    PUBLIC_HEADERS
    ${geometry_public_headers})
opengeolab_copy_runtime_dlls(ogl_geometry)

if(OPENGEOLAB_BUILD_TESTS)
  opengeolab_add_doctest_test(
      ogl_geometry_bounding_box_calculator_test
      SOURCES
      tests/bounding_box_calculator_test.cpp
      LINKS
      OpenGeoLab::Geometry
      CTEST_NAME
      ogl_geometry_bounding_box_calculator_test)
endif()
