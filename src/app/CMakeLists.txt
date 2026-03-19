qt_add_executable(
  opengeolab_app
  src/main.cpp
  src/AppController.cpp
  include/opengeolab/app/AppController.hpp
)

target_compile_features(opengeolab_app PRIVATE cxx_std_20)
target_include_directories(
  opengeolab_app
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(
  opengeolab_app
  PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets
    OpenGeoLab::Python
)

qt_add_qml_module(
  opengeolab_app
  URI OpenGeoLab.App
  VERSION 1.0
  QML_FILES
    resource/qml/Main.qml
)

if(NOT TARGET opengeolab_pywrapper)
  message(
    FATAL_ERROR
      "opengeolab_app requires the opengeolab_pywrapper module. Enable OPENGEOLAB_BUILD_PYTHON_WRAPPER."
  )
endif()
add_dependencies(opengeolab_app opengeolab_pywrapper)

set_target_properties(
  opengeolab_app
  PROPERTIES
    FOLDER "app"
    WIN32_EXECUTABLE TRUE
)

add_custom_command(
  TARGET opengeolab_app
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:opengeolab_app>/python"
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:opengeolab_app>/plugins/python"
  COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/resource/python/opengeolab_runtime.py"
    "$<TARGET_FILE_DIR:opengeolab_app>/python/opengeolab_runtime.py"
  COMMAND
    ${CMAKE_COMMAND} -E copy_directory
    "${PROJECT_SOURCE_DIR}/plugins/python"
    "$<TARGET_FILE_DIR:opengeolab_app>/plugins/python"
  VERBATIM
)

install(
  TARGETS opengeolab_app
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
