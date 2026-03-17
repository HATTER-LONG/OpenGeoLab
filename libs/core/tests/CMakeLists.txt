ogl_add_catch2_test(opengeolab_core_test
  SOURCES test_action_execution_utilities.cpp
  LINKS   ogl_core
)

ogl_add_catch2_test(opengeolab_action_service_registration_test
  SOURCES test_action_service_registration.cpp
  LINKS   ogl_core
)
