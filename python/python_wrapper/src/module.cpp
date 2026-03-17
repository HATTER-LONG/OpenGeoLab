#include <ogl/python_wrapper/OpenGeoLabPythonBridge.hpp>
#include <ogl/python_wrapper/PythonJsonInterop.hpp>

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(opengeolab, module) {
    module.doc() = "OpenGeoLab Python bridge for modular component routing";

    py::class_<OGL::PythonWrapper::OpenGeoLabPythonBridge>(module, "OpenGeoLabPythonBridge")
        .def(py::init<>())
        .def(
            "process",
            [](const OGL::PythonWrapper::OpenGeoLabPythonBridge& bridge,
               const py::object& request) {
                return bridge.process(OGL::PythonWrapper::parsePythonJsonArgument(request)).dump(2);
            },
            py::arg("request"));

    module.def(
        "process",
        [](const py::object& request) {
            OGL::PythonWrapper::OpenGeoLabPythonBridge bridge;
            return bridge.process(OGL::PythonWrapper::parsePythonJsonArgument(request)).dump(2);
        },
        py::arg("request"));
}
