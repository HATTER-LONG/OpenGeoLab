set(geometry_sources
  src/GeometryService.cpp
)

set(geometry_public_headers
  include/opengeolab/geometry/GeometryService.hpp
)

opengeolab_add_module(
  ogl_geometry
  ALIAS_NAME
    Geometry
  SOURCES
    ${geometry_sources}
  PUBLIC_HEADERS
    ${geometry_public_headers}
  PUBLIC_LINKS
    fmt::fmt
    nlohmann_json::nlohmann_json
)
