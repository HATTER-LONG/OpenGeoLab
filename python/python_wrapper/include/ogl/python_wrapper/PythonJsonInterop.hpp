/**
 * @file PythonJsonInterop.hpp
 * @brief Shared pybind11 and JSON conversion helpers for OpenGeoLab request entrypoints.
 */

#pragma once

#include <nlohmann/json.hpp>
#include <pybind11/pybind11.h>

namespace OGL::PythonWrapper {

/**
 * @brief Convert a Python request value into JSON for downstream request parsing.
 * @param value Python object supplied by embedded or external Python callers.
 * @return JSON representation of the Python input.
 */
auto parsePythonJsonArgument(const pybind11::object& value) -> nlohmann::json;

/**
 * @brief Convert JSON back into a Python object for embedded-Python responses.
 * @param value JSON response payload.
 * @return Python object materialized with the stdlib `json` module.
 */
auto toPythonJson(const nlohmann::json& value) -> pybind11::object;

} // namespace OGL::PythonWrapper
