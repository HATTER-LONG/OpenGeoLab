#include <ogl/python_wrapper/PythonJsonInterop.hpp>

#include <string>

namespace py = pybind11;

namespace OGL::PythonWrapper {

auto parsePythonJsonArgument(const py::object& value) -> nlohmann::json {
    if(value.is_none()) {
        return nlohmann::json::object();
    }

    if(py::isinstance<py::str>(value)) {
        const auto requestJson = value.cast<std::string>();
        return requestJson.empty() ? nlohmann::json::object() : nlohmann::json::parse(requestJson);
    }

    const auto jsonModule = py::module_::import("json");
    return nlohmann::json::parse(jsonModule.attr("dumps")(value).cast<std::string>());
}

auto toPythonJson(const nlohmann::json& value) -> py::object {
    return py::module_::import("json").attr("loads")(value.dump());
}

} // namespace OGL::PythonWrapper
