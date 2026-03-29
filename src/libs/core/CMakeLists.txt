set(core_public_headers
    include/opengeolab/core/action.hpp
    include/opengeolab/core/entity_tag.hpp
    include/opengeolab/core/logger.hpp
    include/opengeolab/core/module.hpp
    include/opengeolab/core/module_data_event.hpp
    include/opengeolab/core/progress_callback.hpp
    include/opengeolab/core/point_vector3d.hpp
    include/opengeolab/core/visual_data.hpp
    include/opengeolab/core/shape_color_palette.hpp)

set(core_sources src/action.cpp src/logger.cpp src/module.cpp
                 src/process_callback.cpp src/shape_color_palette.cpp)
opengeolab_add_module(
    opengeolab_core
    ALIAS_NAME
    Core
    SOURCES
    ${core_sources}
    PUBLIC_HEADERS
    ${core_public_headers}
    PUBLIC_LINKS
    Kangaroo::Kangaroo
    spdlog::spdlog
    nlohmann_json::nlohmann_json)

if (OPENGEOLAB_BUILD_TESTS)
    opengeolab_add_doctest_test(
        opengeolab_core_test SOURCES test/point_vector3d_test.cpp LINKS
        OpenGeoLab::Core)
endif ()
