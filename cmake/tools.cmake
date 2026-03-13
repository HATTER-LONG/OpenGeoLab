# this file contains a list of tools that can be activated and downloaded on-demand each tool is
# enabled during configuration by passing an additional `-DUSE_<TOOL>=<VALUE>` argument to CMake

function(opengeolab_resolve_package package_name target_name)
  set(options)
  set(oneValueArgs VERSION GITHUB_REPOSITORY GIT_TAG SOURCE_DIR)
  set(multiValueArgs CPM_OPTIONS)
  cmake_parse_arguments(OPENGEOLAB_PKG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(TARGET ${target_name})
    return()
  endif()

  find_package(${package_name} ${OPENGEOLAB_PKG_VERSION} CONFIG QUIET)
  if(TARGET ${target_name})
    message(STATUS "Using installed ${package_name} package")
    return()
  endif()

  if(NOT OPENGEOLAB_FETCH_DEPENDENCIES)
    message(
      FATAL_ERROR
        "${package_name} was not found. Provide ${target_name}, install ${package_name}, or enable OPENGEOLAB_FETCH_DEPENDENCIES."
    )
  endif()

  message(STATUS "Fetching ${package_name} from GitHub")
  CPMAddPackage(
    NAME ${package_name}
    GITHUB_REPOSITORY ${OPENGEOLAB_PKG_GITHUB_REPOSITORY}
    GIT_TAG ${OPENGEOLAB_PKG_GIT_TAG}
    OPTIONS ${OPENGEOLAB_PKG_CPM_OPTIONS}
  )

  if(NOT TARGET ${target_name})
    message(FATAL_ERROR "Failed to make ${target_name} available")
  endif()
endfunction()

# only activate tools for top level project
if(NOT PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  return()
endif()

include(${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)

# enables sanitizers support using the the `USE_SANITIZER` flag available values are: Address,
# Memory, MemoryWithOrigins, Undefined, Thread, Leak, 'Address;Undefined'
if(USE_SANITIZER OR USE_STATIC_ANALYZER)
  CPMAddPackage("gh:StableCoder/cmake-scripts#24.04")

  if(USE_SANITIZER)
    include(${cmake-scripts_SOURCE_DIR}/sanitizers.cmake)
  endif()

  if(USE_STATIC_ANALYZER)
    if("clang-tidy" IN_LIST USE_STATIC_ANALYZER)
      set(CLANG_TIDY
          ON
          CACHE INTERNAL ""
      )
    else()
      set(CLANG_TIDY
          OFF
          CACHE INTERNAL ""
      )
    endif()
    if("iwyu" IN_LIST USE_STATIC_ANALYZER)
      set(IWYU
          ON
          CACHE INTERNAL ""
      )
    else()
      set(IWYU
          OFF
          CACHE INTERNAL ""
      )
    endif()
    if("cppcheck" IN_LIST USE_STATIC_ANALYZER)
      set(CPPCHECK
          ON
          CACHE INTERNAL ""
      )
    else()
      set(CPPCHECK
          OFF
          CACHE INTERNAL ""
      )
    endif()

    include(${cmake-scripts_SOURCE_DIR}/tools.cmake)

    if(${CLANG_TIDY})
      clang_tidy(${CLANG_TIDY_ARGS})
    endif()

    if(${IWYU})
      include_what_you_use(${IWYU_ARGS})
    endif()

    if(${CPPCHECK})
      cppcheck(${CPPCHECK_ARGS})
    endif()
  endif()
endif()

# enables CCACHE support through the USE_CCACHE flag possible values are: YES, NO or equivalent
if(USE_CCACHE)
  CPMAddPackage("gh:TheLartians/Ccache.cmake@1.2.4")
endif()
