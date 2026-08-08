# SetupPySideVenv.cmake Creates a project-local Python venv, installs PySide6
# with strict Qt version matching, and copies platform-specific runtime
# dependencies.

# Extract Qt6 major.minor version for PySide6 version pinning.
set(OPENGEOLAB_QT_MAJOR ${Qt6_VERSION_MAJOR})
set(OPENGEOLAB_QT_MINOR ${Qt6_VERSION_MINOR})
set(PYSIDE6_REQUIRED_VERSION "${OPENGEOLAB_QT_MAJOR}.${OPENGEOLAB_QT_MINOR}")

# Place venv in the source tree so it survives build directory clean.
set(OPENGEOLAB_PYVENV_DIR
    "${PROJECT_SOURCE_DIR}/pyvenv"
    CACHE PATH "Python venv for PySide6 (persists outside build/)")

# --- Platform-dependent paths inside the venv --------------------------------
if (WIN32)
    set(_pyvenv_python "${OPENGEOLAB_PYVENV_DIR}/Scripts/python.exe")
    set(_pyvenv_pip "${OPENGEOLAB_PYVENV_DIR}/Scripts/pip.exe")
else ()
    set(_pyvenv_python "${OPENGEOLAB_PYVENV_DIR}/bin/python")
    set(_pyvenv_pip "${OPENGEOLAB_PYVENV_DIR}/bin/pip")
endif ()

# --- Create venv if absent ---------------------------------------------------
if (NOT EXISTS "${OPENGEOLAB_PYVENV_DIR}/pyvenv.cfg")
    message(STATUS "Creating Python venv at ${OPENGEOLAB_PYVENV_DIR}...")
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -m venv "${OPENGEOLAB_PYVENV_DIR}"
        RESULT_VARIABLE venv_result)
    if (NOT venv_result EQUAL 0)
        message(
            FATAL_ERROR
                "Failed to create Python venv (exit code ${venv_result})")
    endif ()
endif ()

if (NOT EXISTS "${_pyvenv_python}")
    message(FATAL_ERROR "Python executable is missing from venv: ${_pyvenv_python}")
endif ()

# A venv survives build-directory cleanup, so verify that it still matches the
# interpreter selected for this configuration before linking embedded Python.
execute_process(
    COMMAND "${_pyvenv_python}" -c
            "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')"
    RESULT_VARIABLE _pyvenv_version_result
    OUTPUT_VARIABLE _pyvenv_version
    ERROR_VARIABLE _pyvenv_version_error
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if (NOT _pyvenv_version_result EQUAL 0)
    message(
        FATAL_ERROR
            "Failed to query Python version from ${_pyvenv_python}: ${_pyvenv_version_error}"
    )
endif ()

set(_python_required_version
    "${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}")
if (NOT "${_pyvenv_version}" STREQUAL "${_python_required_version}")
    message(
        FATAL_ERROR
            "Python venv version mismatch: ${_pyvenv_python} uses Python ${_pyvenv_version}, "
            "but CMake selected Python ${_python_required_version}. Remove ${OPENGEOLAB_PYVENV_DIR} "
            "and configure again, or select the matching Python3_EXECUTABLE.")
endif ()

# Let Python report the install scheme rather than duplicating platform-specific
# venv layout rules in CMake.
execute_process(
    COMMAND "${_pyvenv_python}" -c
            "import sysconfig; print(sysconfig.get_path('purelib'))"
    RESULT_VARIABLE _pyvenv_site_result
    OUTPUT_VARIABLE _pyvenv_site_packages
    ERROR_VARIABLE _pyvenv_site_error
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if (NOT _pyvenv_site_result EQUAL 0 OR _pyvenv_site_packages STREQUAL "")
    message(
        FATAL_ERROR
            "Failed to query site-packages from ${_pyvenv_python}: ${_pyvenv_site_error}"
    )
endif ()

# Build-time Python generators (notably glad) must use the project venv too.
# Keep Python3::Python bound to the interpreter selected by find_package while
# exposing the venv executable separately for tools and the embedded runtime.
set(OPENGEOLAB_PYTHON_EXECUTABLE
    "${_pyvenv_python}"
    CACHE FILEPATH "Python executable used by the embedded runtime" FORCE)
set(Python_EXECUTABLE
    "${_pyvenv_python}"
    CACHE FILEPATH "Python executable used by build-time generators" FORCE)

# --- Install PySide6 with strict major.minor matching ------------------------
execute_process(
    COMMAND "${_pyvenv_pip}" show PySide6
    OUTPUT_VARIABLE pyside_info
    ERROR_QUIET
    RESULT_VARIABLE pip_show_result)

set(_need_install TRUE)
if (pip_show_result EQUAL 0)
    string(REGEX MATCH "Version: ([0-9]+)\\.([0-9]+)" _pyside_match
                 "${pyside_info}")
    if ("${CMAKE_MATCH_1}.${CMAKE_MATCH_2}" STREQUAL
        "${PYSIDE6_REQUIRED_VERSION}")
        set(_need_install FALSE)
        message(
            STATUS
                "PySide6 ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.x already installed, skipping."
        )
    endif ()
endif ()

if (_need_install)
    message(
        STATUS "Installing PySide6==${PYSIDE6_REQUIRED_VERSION}.* into venv...")
    execute_process(
        COMMAND "${_pyvenv_pip}" install
                "PySide6==${PYSIDE6_REQUIRED_VERSION}.*" --quiet
        RESULT_VARIABLE pip_result)
    if (NOT pip_result EQUAL 0)
        message(
            FATAL_ERROR
                "Failed to install PySide6==${PYSIDE6_REQUIRED_VERSION}.* (exit code ${pip_result})"
        )
    endif ()

    execute_process(COMMAND "${_pyvenv_pip}" show PySide6
                    OUTPUT_VARIABLE pyside_verify_info)
    string(REGEX MATCH "Version: ([0-9]+)\\.([0-9]+)" _verify_match
                 "${pyside_verify_info}")
    if (NOT "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}" STREQUAL
        "${PYSIDE6_REQUIRED_VERSION}")
        message(
            FATAL_ERROR
                "PySide6 version mismatch! "
                "Expected ${PYSIDE6_REQUIRED_VERSION}.x but got ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.x. "
                "Qt version is ${Qt6_VERSION}.")
    endif ()
    message(
        STATUS
            "PySide6 ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.x installed successfully."
    )
endif ()

# --- Install Python build dependencies ---------------------------------------
# glad imports Jinja2 when generating the OpenGL loader during the build.
execute_process(
    COMMAND "${_pyvenv_pip}" install "jinja2>=3.1" --quiet
    RESULT_VARIABLE _build_package_result)
if (NOT _build_package_result EQUAL 0)
    message(
        FATAL_ERROR
            "Failed to install jinja2 (exit code ${_build_package_result}); "
            "the glad code generator cannot run without it.")
endif ()

# --- Install additional Python packages for AI Chat plugin -------------------
# pip install is idempotent: it checks locally first and returns immediately if
# the requirement (including version constraint) is already satisfied — no
# network access needed in that case.
set(_extra_packages "github-copilot-sdk" "markdown>=3.6")

foreach (_pkg IN LISTS _extra_packages)
    string(REGEX REPLACE "[>=<].*" "" _pkg_name "${_pkg}")
    execute_process(
        COMMAND "${_pyvenv_pip}" install "${_pkg}" --quiet
        RESULT_VARIABLE _pkg_install_result)
    if (NOT _pkg_install_result EQUAL 0)
        message(
            WARNING
                "Failed to install ${_pkg} (exit code ${_pkg_install_result}). "
                "AI Chat plugin may not function correctly.")
    endif ()
endforeach ()

# --- Copy python3.dll (Windows only) -----------------------------------------
# shiboken6.abi3.dll uses the Python stable ABI and depends on python3.dll,
# which is NOT a link dependency of any CMake target — TARGET_RUNTIME_DLLS
# cannot capture it.
if (WIN32)
    cmake_path(GET Python3_EXECUTABLE PARENT_PATH _python_home)
    set(_python3_dll "${_python_home}/python3.dll")
    if (EXISTS "${_python3_dll}")
        file(COPY "${_python3_dll}"
             DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
        message(
            STATUS "Copied python3.dll to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    else ()
        message(WARNING "python3.dll not found at ${_python3_dll} — "
                        "shiboken6 (PySide6) may fail to load at runtime.")
    endif ()
endif ()

# --- Export venv site-packages path for compile definitions -------------------
file(TO_CMAKE_PATH "${_pyvenv_site_packages}" _pyvenv_site_cmake)
set(OPENGEOLAB_PYVENV_SITE_PACKAGES
    "${_pyvenv_site_cmake}"
    CACHE PATH "Path to PySide6 venv site-packages" FORCE)
