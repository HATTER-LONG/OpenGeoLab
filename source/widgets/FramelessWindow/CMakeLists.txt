project(QWKQuick
    VERSION ${QWINDOWKIT_VERSION}
    LANGUAGES CXX
)

# qwindowkit is a Qt abstraction library for frameless window. depends QWindowKit::Core
CPMAddPackage(
  NAME qwindowkit
  GITHUB_REPOSITORY "stdware/qwindowkit"
  GIT_TAG 1.4.0
  OPTIONS "QWINDOWKIT_BUILD_WIDGETS OFF" "QWINDOWKIT_BUILD_EXAMPLES OFF"
          "QWINDOWKIT_BUILD_TESTS OFF" "QWINDOWKIT_BUILD_QUICK OFF" "QWINDOWKIT_BUILD_STATIC ON"
)

set(_src
    qwkquickglobal.h
    qwkquickglobal.cpp
    quickitemdelegate_p.h
    quickitemdelegate.cpp
    quickwindowagent.h
    quickwindowagent_p.h
    quickwindowagent.cpp
)

if(WIN32)
    list(APPEND _src quickwindowagent_win.cpp)
elseif(APPLE)
    list(APPEND _src quickwindowagent_mac.cpp)
endif()
