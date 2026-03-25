set(core_public_headers include/opengeolab/core/progress_callback.hpp
                        include/opengeolab/core/point_vector3d.hpp)

set(core_sources src/process_callback.cpp)
opengeolab_add_module(
    opengeolab_core
    ALIAS_NAME
    Core
    SOURCES
    ${core_sources}
    PUBLIC_HEADERS
    ${core_public_headers}
    PUBLIC_LINKS
    Kangaroo::Kangaroo)

if (OPENGEOLAB_BUILD_TESTS)
    opengeolab_add_doctest_test(
        opengeolab_core_test SOURCES test/point_vector3d_test.cpp LINKS
        OpenGeoLab::Core)
endif ()
