# SetupPySideVenv.cmake
# Creates a project-local Python venv, installs PySide6 with strict Qt version
# matching, and copies platform-specific runtime dependencies.

# Extract Qt6 major.minor version for PySide6 version pinning.
set(OPENGEOLAB_QT_MAJOR ${Qt6_VERSION_MAJOR})
set(OPENGEOLAB_QT_MINOR ${Qt6_VERSION_MINOR})
set(PYSIDE6_REQUIRED_VERSION "${OPENGEOLAB_QT_MAJOR}.${OPENGEOLAB_QT_MINOR}")

# Place venv in the source tree so it survives build directory clean.
set(OPENGEOLAB_PYVENV_DIR
    "${PROJECT_SOURCE_DIR}/pyvenv"
    CACHE PATH "Python venv for PySide6 (persists outside build/)")

# --- Platform-dependent paths inside the venv --------------------------------
if(WIN32)
    set(_pyvenv_pip "${OPENGEOLAB_PYVENV_DIR}/Scripts/pip.exe")
    set(_pyvenv_site_packages "${OPENGEOLAB_PYVENV_DIR}/Lib/site-packages")
else()
    set(_pyvenv_pip "${OPENGEOLAB_PYVENV_DIR}/bin/pip")
    set(_pyvenv_site_packages
        "${OPENGEOLAB_PYVENV_DIR}/lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages"
    )
endif()

# --- Create venv if absent ---------------------------------------------------
if(NOT EXISTS "${OPENGEOLAB_PYVENV_DIR}/pyvenv.cfg")
    message(STATUS "Creating Python venv at ${OPENGEOLAB_PYVENV_DIR}...")
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -m venv "${OPENGEOLAB_PYVENV_DIR}"
        RESULT_VARIABLE venv_result)
    if(NOT venv_result EQUAL 0)
        message(
            FATAL_ERROR
                "Failed to create Python venv (exit code ${venv_result})")
    endif()
endif()

# --- Install PySide6 with strict major.minor matching ------------------------
execute_process(
    COMMAND "${_pyvenv_pip}" show PySide6
    OUTPUT_VARIABLE pyside_info
    ERROR_QUIET
    RESULT_VARIABLE pip_show_result)

set(_need_install TRUE)
if(pip_show_result EQUAL 0)
    string(REGEX MATCH "Version: ([0-9]+)\\.([0-9]+)" _pyside_match
           "${pyside_info}")
    if("${CMAKE_MATCH_1}.${CMAKE_MATCH_2}" STREQUAL
       "${PYSIDE6_REQUIRED_VERSION}")
        set(_need_install FALSE)
        message(
            STATUS
                "PySide6 ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.x already installed, skipping."
        )
    endif()
endif()

if(_need_install)
    message(
        STATUS
            "Installing PySide6==${PYSIDE6_REQUIRED_VERSION}.* into venv...")
    execute_process(
        COMMAND "${_pyvenv_pip}" install
                "PySide6==${PYSIDE6_REQUIRED_VERSION}.*" --quiet
        RESULT_VARIABLE pip_result)
    if(NOT pip_result EQUAL 0)
        message(
            FATAL_ERROR
                "Failed to install PySide6==${PYSIDE6_REQUIRED_VERSION}.* (exit code ${pip_result})"
        )
    endif()

    execute_process(
        COMMAND "${_pyvenv_pip}" show PySide6
        OUTPUT_VARIABLE pyside_verify_info)
    string(REGEX MATCH "Version: ([0-9]+)\\.([0-9]+)" _verify_match
           "${pyside_verify_info}")
    if(NOT "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}" STREQUAL
       "${PYSIDE6_REQUIRED_VERSION}")
        message(
            FATAL_ERROR
                "PySide6 version mismatch! "
                "Expected ${PYSIDE6_REQUIRED_VERSION}.x but got ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.x. "
                "Qt version is ${Qt6_VERSION}.")
    endif()
    message(
        STATUS
            "PySide6 ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.x installed successfully."
    )
endif()

# --- Copy python3.dll (Windows only) -----------------------------------------
# shiboken6.abi3.dll uses the Python stable ABI and depends on python3.dll,
# which is NOT a link dependency of any CMake target — TARGET_RUNTIME_DLLS
# cannot capture it.
if(WIN32)
    cmake_path(GET Python3_EXECUTABLE PARENT_PATH _python_home)
    set(_python3_dll "${_python_home}/python3.dll")
    if(EXISTS "${_python3_dll}")
        file(COPY "${_python3_dll}"
             DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
        message(
            STATUS "Copied python3.dll to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    else()
        message(
            WARNING
                "python3.dll not found at ${_python3_dll} — "
                "shiboken6 (PySide6) may fail to load at runtime.")
    endif()
endif()

# --- Export venv site-packages path for compile definitions -------------------
file(TO_CMAKE_PATH "${_pyvenv_site_packages}" _pyvenv_site_cmake)
set(OPENGEOLAB_PYVENV_SITE_PACKAGES
    "${_pyvenv_site_cmake}"
    CACHE PATH "Path to PySide6 venv site-packages" FORCE)
