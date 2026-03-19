# Centralized doctest target helper for OpenGeoLab modules.
#
# Usage:
#   opengeolab_add_doctest_test(<target>
#     SOURCES <source1> [<source2> ...]
#     LINKS   <lib1> [<lib2> ...]
#     [DEFINITIONS <definition1> [<definition2> ...]]
#     [CTEST_NAME <name>]
#   )

function(opengeolab_add_doctest_test target)
  cmake_parse_arguments(ARG "" "CTEST_NAME" "SOURCES;LINKS;DEFINITIONS" ${ARGN})

  if(NOT ARG_SOURCES)
    message(FATAL_ERROR "opengeolab_add_doctest_test: SOURCES is required for target '${target}'")
  endif()

  set(ctest_name ${target})
  if(ARG_CTEST_NAME)
    set(ctest_name ${ARG_CTEST_NAME})
  endif()

  add_executable(${target} ${ARG_SOURCES})
  target_compile_features(${target} PRIVATE cxx_std_20)
  target_link_libraries(${target} PRIVATE doctest::doctest_with_main ${ARG_LINKS})

  if(ARG_DEFINITIONS)
    target_compile_definitions(${target} PRIVATE ${ARG_DEFINITIONS})
  endif()

  add_test(NAME ${ctest_name} COMMAND $<TARGET_FILE:${target}>)
  set_target_properties(${target} PROPERTIES FOLDER "tests")
endfunction()
