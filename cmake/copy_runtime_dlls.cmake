if (NOT DEFINED runtime_dlls OR runtime_dlls STREQUAL "")
    return()
endif ()

string(REPLACE "|" ";" runtime_dll_list "${runtime_dlls}")

execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${runtime_dll_list} ${target_dir}
    COMMAND_ERROR_IS_FATAL ANY)
