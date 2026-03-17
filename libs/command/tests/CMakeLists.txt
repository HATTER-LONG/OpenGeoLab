ogl_add_catch2_test(opengeolab_command_test
  SOURCES test_command.cpp
  LINKS   ogl_command
)

ogl_add_catch2_test(opengeolab_command_protocol_test
  SOURCES test_request_protocol.cpp
  LINKS   ogl_command
)
