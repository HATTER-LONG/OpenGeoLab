set(geometry_public_headers include/opengeolab/geometry/geometry_module.hpp)

set(geometry_sources src/geometry_module.cpp src/shape_store.cpp
                     src/tessellator.cpp src/occ_primitives.cpp)

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
    OpenGeoLab::Scene
    PRIVATE_LINKS
    nlohmann_json::nlohmann_json
    TKernel
    TKMath
    TKBRep
    TKTopAlgo
    TKPrim
    TKMesh
    TKG3d)

if (OPENGEOLAB_BUILD_TESTS)
    opengeolab_add_doctest_test(
        opengeolab_geometry_module_test
        SOURCES
        tests/geometry_module_test.cpp
        src/shape_store.cpp
        src/tessellator.cpp
        src/occ_primitives.cpp
        LINKS
        OpenGeoLab::Geometry
        nlohmann_json::nlohmann_json
        TKernel
        TKBRep
        TKPrim
        TKMath
        TKTopAlgo
        TKMesh
        TKG3d)
    target_include_directories(opengeolab_geometry_module_test
                               PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
endif ()
