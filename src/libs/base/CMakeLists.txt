set(base_public_headers include/opengeolab/base/notification_sink.hpp
                        include/opengeolab/base/notification_registry.hpp)

set(base_sources src/notification_registry.cpp)

opengeolab_add_module(
    opengeolab_base
    ALIAS_NAME
    Base
    SOURCES
    ${base_sources}
    PUBLIC_HEADERS
    ${base_public_headers})

if (OPENGEOLAB_BUILD_TESTS)
    opengeolab_add_doctest_test(
        opengeolab_base_notification_sink_test SOURCES
        tests/notification_sink_test.cpp LINKS OpenGeoLab::Base)
endif ()
