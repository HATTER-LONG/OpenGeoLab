# Centralized Catch2 test target helper for OpenGeoLab modules.
#
# Usage:
#   ogl_add_catch2_test(<target>
#     SOURCES <source1> [<source2> ...]
#     LINKS   <lib1> [<lib2> ...]
#     [CTEST_NAME <name>]
#   )
#
# Adds the executable, sets cxx_std_20, links Catch2::Catch2WithMain plus any
# additional LINKS, and registers the target with CTest.  CTEST_NAME defaults to
# <target> when omitted.  Callers that need AUTOMOC, extra compile definitions,
# link options, or include directories should append those after the call using
# standard CMake target commands.
function(ogl_add_catch2_test target)
  cmake_parse_arguments(ARG "" "CTEST_NAME" "SOURCES;LINKS" ${ARGN})

  if(NOT ARG_SOURCES)
    message(FATAL_ERROR "ogl_add_catch2_test: SOURCES is required for target '${target}'")
  endif()

  set(ctest_name ${target})
  if(ARG_CTEST_NAME)
    set(ctest_name ${ARG_CTEST_NAME})
  endif()

  add_executable(${target} ${ARG_SOURCES})
  target_compile_features(${target} PRIVATE cxx_std_20)
  target_link_libraries(${target} PRIVATE Catch2::Catch2WithMain ${ARG_LINKS})
  add_test(NAME ${ctest_name} COMMAND ${target})
endfunction()
