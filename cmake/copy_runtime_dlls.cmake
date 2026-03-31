# copy_runtime_dlls.cmake — Copy runtime DLLs to the target output directory.
# Called as a post-build script with -Druntime_dlls and -Dtarget_dir. Accepts
# both pipe-delimited (legacy) and semicolon-delimited lists.

if (NOT DEFINED runtime_dlls OR runtime_dlls STREQUAL "")
    return()
endif ()

# Normalise: accept "|" or ";" as separator.
string(REPLACE "|" ";" runtime_dll_list "${runtime_dlls}")

foreach (_dll IN LISTS runtime_dll_list)
    if (EXISTS "${_dll}")
        execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_dll}"
                                "${target_dir}" COMMAND_ERROR_IS_FATAL ANY)
    endif ()
endforeach ()
