include_guard(GLOBAL)
include(CMakeParseArguments)
include(GenerateExportHeader)
include(GNUInstallDirs)

function (opengeolab_add_module target)
    set(options)
    set(oneValueArgs ALIAS_NAME)
    set(multiValueArgs SOURCES PUBLIC_HEADERS PUBLIC_LINKS PRIVATE_LINKS
                       PUBLIC_DEFINITIONS PRIVATE_DEFINITIONS)
    cmake_parse_arguments(MODULE "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN})

    if (NOT MODULE_ALIAS_NAME)
        message(
            FATAL_ERROR
                "opengeolab_add_module requires ALIAS_NAME for target ${target}"
        )
    endif ()

    set(module_library_type STATIC)
    if (DEFINED OPENGEOLAB_LIBRARY_TYPE)
        set(module_library_type ${OPENGEOLAB_LIBRARY_TYPE})
    endif ()

    add_library(${target} ${module_library_type})
    add_library(OpenGeoLab::${MODULE_ALIAS_NAME} ALIAS ${target})

    string(TOLOWER "${MODULE_ALIAS_NAME}" module_alias_name_lower)
    string(TOUPPER "${MODULE_ALIAS_NAME}" module_alias_name_upper)
    set(module_generated_include_dir
        "${CMAKE_CURRENT_BINARY_DIR}/generated/include")
    set(module_generated_export_header
        "${module_generated_include_dir}/opengeolab/${module_alias_name_lower}/${module_alias_name_lower}_export.hpp"
    )
    set(module_export_macro_name "OPENGEOLAB_${module_alias_name_upper}_EXPORT")
    set(module_static_define_name
        "OPENGEOLAB_${module_alias_name_upper}_STATIC_DEFINE")
    file(
        MAKE_DIRECTORY
        "${module_generated_include_dir}/opengeolab/${module_alias_name_lower}")
    generate_export_header(
        ${target}
        EXPORT_FILE_NAME
        "${module_generated_export_header}"
        EXPORT_MACRO_NAME
        ${module_export_macro_name}
        STATIC_DEFINE
        ${module_static_define_name})

    target_sources(
        ${target}
        PRIVATE ${MODULE_SOURCES}
        PUBLIC FILE_SET
               HEADERS
               BASE_DIRS
               "${CMAKE_CURRENT_SOURCE_DIR}/include"
               "${module_generated_include_dir}"
               FILES
               ${MODULE_PUBLIC_HEADERS}
               "${module_generated_export_header}")

    target_compile_features(${target} PUBLIC cxx_std_20)
    target_include_directories(
        ${target}
        PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
               "$<BUILD_INTERFACE:${module_generated_include_dir}>"
               "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

    if (module_library_type STREQUAL "STATIC")
        target_compile_definitions(${target}
                                   PUBLIC ${module_static_define_name})
    endif ()

    if (MODULE_PUBLIC_LINKS OR MODULE_PRIVATE_LINKS)
        target_link_libraries(
            ${target}
            PUBLIC ${MODULE_PUBLIC_LINKS}
            PRIVATE ${MODULE_PRIVATE_LINKS})
    endif ()

    if (MODULE_PUBLIC_DEFINITIONS OR MODULE_PRIVATE_DEFINITIONS)
        target_compile_definitions(
            ${target}
            PUBLIC ${MODULE_PUBLIC_DEFINITIONS}
            PRIVATE ${MODULE_PRIVATE_DEFINITIONS})
    endif ()

    set_target_properties(${target} PROPERTIES EXPORT_NAME ${MODULE_ALIAS_NAME}
                                               FOLDER "lib")

    install(
        TARGETS ${target}
        EXPORT OpenGeoLabTargets
        FILE_SET HEADERS
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endfunction ()

function (opengeolab_copy_runtime_dlls target)
    if (NOT TARGET ${target})
        message(
            FATAL_ERROR
                "opengeolab_copy_runtime_dlls requires an existing target: ${target}"
        )
    endif ()

    if (NOT WIN32)
        return()
    endif ()

    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${target}> $<TARGET_FILE_DIR:${target}>
        COMMAND_EXPAND_LISTS VERBATIM)
endfunction ()
