set(io_public_headers include/opengeolab/io/io_module.hpp
                      include/opengeolab/io/read_brep_action.hpp)
set(io_sources src/io_module.cpp src/read_brep_action.cpp)

opengeolab_add_module(
    opengeolab_io
    ALIAS_NAME
    IO
    SOURCES
    ${io_sources}
    PUBLIC_HEADERS
    ${io_public_headers}
    PUBLIC_LINKS
    OpenGeoLab::Core)

if (OPENGEOLAB_BUILD_TESTS)
    opengeolab_add_doctest_test(opengeolab_io_test SOURCES
                                test/io_module_test.cpp LINKS OpenGeoLab::IO)
endif ()
