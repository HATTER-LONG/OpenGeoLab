#include "opengeolab/python_embed/embedded_python_runtime.hpp"

#include <doctest/doctest.h>

#include <concepts>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <string_view>

namespace OpenGeoLab::PythonEmbed::Tests {

static_assert(std::is_same_v<ProgressCallback, Core::ProgressCallback>);

static_assert(requires(EmbeddedPythonRuntime& runtime,
                       std::string_view request_json,
                       ProgressCallback progress_callback) {
    { runtime.process(request_json) } -> std::same_as<std::string>;
    { runtime.process(request_json, progress_callback) } -> std::same_as<std::string>;
});

TEST_CASE("EmbeddedPythonRuntime initializes Python and loads a runtime module") {
    const auto test_root =
        std::filesystem::temp_directory_path() / "opengeolab-python-embed-api-test";
    std::filesystem::remove_all(test_root);
    std::filesystem::create_directories(test_root);

    {
        std::ofstream runtime_module(test_root / "opengeolab_runtime.py");
        REQUIRE(runtime_module.good());
        runtime_module << "def process(request_json, progress_callback=None):\n"
                          "    return request_json\n";
    }

    {
        EmbeddedPythonRuntime runtime(test_root, test_root, test_root);
        CHECK(runtime.process(R"({"ok":true})") == R"({"ok":true})");
    }

    std::filesystem::remove_all(test_root);
}

} // namespace OpenGeoLab::PythonEmbed::Tests
