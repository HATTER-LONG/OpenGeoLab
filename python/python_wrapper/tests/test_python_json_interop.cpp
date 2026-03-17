#include <catch2/catch_test_macros.hpp>

#include <ogl/python_wrapper/PythonJsonInterop.hpp>

#include <pybind11/embed.h>

#include <memory>
#include <string>

namespace py = pybind11;

TEST_CASE("python interop normalizes None to an empty object", "[python][interop]") {
    auto interpreter = std::make_unique<py::scoped_interpreter>();

    const auto value = OGL::PythonWrapper::parsePythonJsonArgument(py::none{});
    REQUIRE(value.is_object());
    CHECK(value.empty());
}

TEST_CASE("python interop parses JSON strings and Python containers", "[python][interop]") {
    auto interpreter = std::make_unique<py::scoped_interpreter>();

    SECTION("json strings parse to objects") {
        const auto value = OGL::PythonWrapper::parsePythonJsonArgument(
            py::str(R"({"module":"geometry","action":"inspectModel"})"));
        CHECK(value.at("module") == "geometry");
        CHECK(value.at("action") == "inspectModel");
    }

    SECTION("dict values convert through json serialization") {
        py::dict request;
        request["module"] = "scene";
        request["action"] = "buildScene";
        request["param"] = py::dict();

        const auto value = OGL::PythonWrapper::parsePythonJsonArgument(request);
        CHECK(value.at("module") == "scene");
        REQUIRE(value.at("param").is_object());
    }

    SECTION("list values round-trip through Python conversion") {
        py::list values;
        values.append(1);
        values.append("two");

        const auto json_value = OGL::PythonWrapper::parsePythonJsonArgument(values);
        REQUIRE(json_value.is_array());
        CHECK(json_value.at(0) == 1);

        const auto python_value = OGL::PythonWrapper::toPythonJson(json_value);
        CHECK(py::len(python_value) == 2);
    }
}

TEST_CASE("python interop surfaces malformed and unserializable values", "[python][interop]") {
    auto interpreter = std::make_unique<py::scoped_interpreter>();

    SECTION("malformed JSON strings throw") {
        REQUIRE_THROWS(OGL::PythonWrapper::parsePythonJsonArgument(py::str("{")));
    }

    SECTION("unserializable Python values throw") {
        auto io = py::module_::import("io");
        auto handle = io.attr("StringIO")();
        REQUIRE_THROWS(OGL::PythonWrapper::parsePythonJsonArgument(handle));
    }
}
