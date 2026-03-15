#include <catch2/catch_test_macros.hpp>

#include <pybind11/embed.h>
#include <pybind11/eval.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

namespace py = pybind11;

namespace {

auto testModuleDir() -> std::filesystem::path {
    return std::filesystem::path(OPENGEOLAB_PYTHON_TEST_MODULE_DIR);
}

auto testScriptPath() -> std::filesystem::path {
    return std::filesystem::path(OPENGEOLAB_PYTHON_TEST_SCRIPT_PATH);
}

auto findOpenGeoLabModulePath(const std::filesystem::path& module_dir) -> std::filesystem::path {
    for(const auto& entry : std::filesystem::directory_iterator(module_dir)) {
        if(!entry.is_regular_file()) {
            continue;
        }

        const auto filename = entry.path().filename().string();
        if(filename.rfind("opengeolab", 0) == 0 && entry.path().extension() == ".pyd") {
            return entry.path();
        }
    }

    return {};
}

void preloadOpenGeoLabModule(const std::filesystem::path& module_dir) {
    const auto module_path = findOpenGeoLabModulePath(module_dir);
    if(module_path.empty()) {
        throw std::runtime_error("Unable to locate the opengeolab Python extension module.");
    }

    auto importlib_machinery = py::module_::import("importlib.machinery");
    auto importlib_util = py::module_::import("importlib.util");
    auto sys = py::module_::import("sys");
    auto loader =
        importlib_machinery.attr("ExtensionFileLoader")("opengeolab", module_path.string());
    auto spec = importlib_util.attr("spec_from_loader")("opengeolab", loader);
    auto module = importlib_util.attr("module_from_spec")(spec);
    loader.attr("exec_module")(module);
    sys.attr("modules")["opengeolab"] = module;
}

void configurePythonPaths() {
    auto sys = py::module_::import("sys");
    sys.attr("path").attr("insert")(0, testModuleDir().string());
    sys.attr("path").attr("insert")(0, testScriptPath().parent_path().string());
    preloadOpenGeoLabModule(testModuleDir());
}

} // namespace

TEST_CASE("python bridge module passes the smoke script and direct bridge calls",
          "[python][bridge][smoke]") {
    auto interpreter = std::make_unique<py::scoped_interpreter>();
    configurePythonPaths();

    SECTION("python smoke script succeeds") {
        REQUIRE_NOTHROW(py::eval_file(testScriptPath().string()));
    }

    SECTION("direct bridge process returns recorded payload") {
        auto opengeolab = py::module_::import("opengeolab");
        CHECK_FALSE(py::hasattr(opengeolab, "call"));
        CHECK_FALSE(py::hasattr(opengeolab, "run_command"));
        CHECK_FALSE(py::hasattr(opengeolab, "suggest_placeholder_geometry_script"));
        const auto response_text = opengeolab
                                       .attr("process")(R"JSON({
  "module": "selection",
  "action": "pickEntity",
  "param": {
    "modelName": "PythonBridgeCatch2",
    "bodyCount": 2,
    "viewportWidth": 960,
    "viewportHeight": 540,
    "screenX": 120,
    "screenY": 96,
    "source": "catch2-test"
  }
})JSON")
                                       .cast<std::string>();
        const auto response = nlohmann::json::parse(response_text);

        REQUIRE(response.value("success", false));
        CHECK(response["payload"].value("recordedCommandCount", 0) == 0);
        CHECK(response["payload"]["selectionResult"].value("hitCount", 0) == 1);

        auto bridge = opengeolab.attr("OpenGeoLabPythonBridge")();
        CHECK(py::hasattr(bridge, "process"));
        CHECK_FALSE(py::hasattr(bridge, "call"));
        CHECK_FALSE(py::hasattr(bridge, "run_command"));
        CHECK_FALSE(py::hasattr(bridge, "suggest_placeholder_geometry_script"));
    }
}
