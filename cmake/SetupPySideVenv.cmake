# SetupPySideVenv.cmake
# Creates a build-local Python venv, installs PySide6 with strict Qt version matching,
# strips PySide6's bundled Qt DLLs to force use of the host application's Qt,
# and copies python3.dll (stable ABI) needed by shiboken6.

# Extract Qt6 major.minor version
set(OPENGEOLAB_QT_MAJOR ${Qt6_VERSION_MAJOR})
set(OPENGEOLAB_QT_MINOR ${Qt6_VERSION_MINOR})
set(PYSIDE6_REQUIRED_VERSION "${OPENGEOLAB_QT_MAJOR}.${OPENGEOLAB_QT_MINOR}")

set(OPENGEOLAB_PYVENV_DIR "${CMAKE_BINARY_DIR}/pyvenv")

# Create venv only if not already present
if(NOT EXISTS "${OPENGEOLAB_PYVENV_DIR}/pyvenv.cfg")
  message(STATUS "Creating Python venv at ${OPENGEOLAB_PYVENV_DIR}...")
  execute_process(
    COMMAND "${Python3_EXECUTABLE}" -m venv "${OPENGEOLAB_PYVENV_DIR}"
    RESULT_VARIABLE venv_result)
  if(NOT venv_result EQUAL 0)
    message(FATAL_ERROR "Failed to create Python venv (exit code ${venv_result})")
  endif()
endif()

# pip install PySide6 with strict major.minor matching
set(PYVENV_PIP "${OPENGEOLAB_PYVENV_DIR}/Scripts/pip.exe")

# Check if PySide6 is already installed with correct version
execute_process(
  COMMAND "${PYVENV_PIP}" show PySide6
  OUTPUT_VARIABLE pyside_info
  ERROR_QUIET
  RESULT_VARIABLE pip_show_result)

set(_need_install TRUE)
if(pip_show_result EQUAL 0)
  string(REGEX MATCH "Version: ([0-9]+)\\.([0-9]+)" _pyside_match "${pyside_info}")
  if("${CMAKE_MATCH_1}.${CMAKE_MATCH_2}" STREQUAL "${PYSIDE6_REQUIRED_VERSION}")
    set(_need_install FALSE)
    message(STATUS "PySide6 ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.x already installed, skipping.")
  endif()
endif()

if(_need_install)
  message(STATUS "Installing PySide6==${PYSIDE6_REQUIRED_VERSION}.* into venv...")
  execute_process(
    COMMAND "${PYVENV_PIP}" install "PySide6==${PYSIDE6_REQUIRED_VERSION}.*" --quiet
    RESULT_VARIABLE pip_result)
  if(NOT pip_result EQUAL 0)
    message(FATAL_ERROR
            "Failed to install PySide6==${PYSIDE6_REQUIRED_VERSION}.* (exit code ${pip_result})")
  endif()

  # Verify installed version
  execute_process(
    COMMAND "${PYVENV_PIP}" show PySide6
    OUTPUT_VARIABLE pyside_verify_info)
  string(REGEX MATCH "Version: ([0-9]+)\\.([0-9]+)" _verify_match "${pyside_verify_info}")
  if(NOT "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}" STREQUAL "${PYSIDE6_REQUIRED_VERSION}")
    message(FATAL_ERROR
            "PySide6 version mismatch! "
            "Expected ${PYSIDE6_REQUIRED_VERSION}.x but got ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.x. "
            "Qt version is ${Qt6_VERSION}.")
  endif()
  message(STATUS "PySide6 ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.x installed successfully.")
endif()

# ---------------------------------------------------------------------------
# Ensure the host application's Qt DLLs take priority over PySide6's bundled
# copies. We do NOT strip PySide6's Qt DLLs (they serve as fallback and are
# needed when PySide6 modules load before the host app's Qt). Instead, the
# embedded runtime adds the application directory to os.add_dll_directory()
# before importing PySide6, so the already-loaded host Qt wins.
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# Copy python3.dll (stable ABI shim) to the runtime output directory.
# shiboken6.abi3.dll depends on python3.dll, which is not a direct link
# dependency of any CMake target and thus not captured by TARGET_RUNTIME_DLLS.
# ---------------------------------------------------------------------------
cmake_path(GET Python3_EXECUTABLE PARENT_PATH _python_home)
set(_python3_dll "${_python_home}/python3.dll")
if(EXISTS "${_python3_dll}")
  file(COPY "${_python3_dll}" DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
  message(STATUS "Copied python3.dll to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
else()
  message(WARNING "python3.dll not found at ${_python3_dll} — "
                  "shiboken6 (PySide6) may fail to load at runtime.")
endif()

# Export venv site-packages path for compile definitions
set(OPENGEOLAB_PYVENV_SITE_PACKAGES "${OPENGEOLAB_PYVENV_DIR}/Lib/site-packages"
    CACHE PATH "Path to PySide6 venv site-packages" FORCE)

# TODO: Cross-platform support — Linux/macOS use bin/pip and lib/python3.x/site-packages
