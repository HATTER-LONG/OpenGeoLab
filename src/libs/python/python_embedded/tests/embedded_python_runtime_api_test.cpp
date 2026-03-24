#include <doctest/doctest.h>
#include <opengeolab/python/embedded_python_runtime.hpp>

#include <concepts>
#include <functional>
#include <string>
#include <string_view>

namespace OpenGeoLab::Python::Tests {

static_assert(std::is_same_v<ProgressCallback, std::function<void(double, std::string_view)>>);

static_assert(requires(EmbeddedPythonRuntime& runtime,
                       std::string_view request_json,
                       ProgressCallback progress_callback) {
    { runtime.process(request_json) } -> std::same_as<std::string>;
    { runtime.process(request_json, progress_callback) } -> std::same_as<std::string>;
});

TEST_CASE("EmbeddedPythonRuntime exposes a progress callback process overload") { CHECK(true); }

} // namespace OpenGeoLab::Python::Tests
