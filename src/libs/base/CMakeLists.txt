set(base_sources src/response_builder.cpp)

set(base_public_headers
    include/opengeolab/base/command_result.hpp
    include/opengeolab/base/action_interface.hpp
    include/opengeolab/base/module_service_interface.hpp
    include/opengeolab/base/protocol_constants.hpp
    include/opengeolab/base/registration_helper.hpp
    include/opengeolab/base/response_builder.hpp)

opengeolab_add_module(
    ogl_base
    ALIAS_NAME
    Base
    SOURCES
    ${base_sources}
    PUBLIC_HEADERS
    ${base_public_headers}
    PUBLIC_LINKS
    Kangaroo::Kangaroo
    nlohmann_json::nlohmann_json)

if(OPENGEOLAB_BUILD_TESTS)
  opengeolab_add_doctest_test(
      ogl_base_registration_helper_test
      SOURCES
      tests/registration_helper_test.cpp
      LINKS
      OpenGeoLab::Base
      Kangaroo::Kangaroo
      CTEST_NAME
      ogl_base_registration_helper_test)
  opengeolab_add_doctest_test(
      ogl_base_test
      SOURCES
      tests/response_builder_test.cpp
      LINKS
      OpenGeoLab::Base
      CTEST_NAME
      ogl_base_test)
endif()
