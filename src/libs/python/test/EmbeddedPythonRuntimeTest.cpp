#include <opengeolab/python/EmbeddedPythonRuntime.hpp>

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

#include <filesystem>

namespace
{

[[nodiscard]] nlohmann::json processRequest(
    OpenGeoLab::Python::EmbeddedPythonRuntime& runtime,
    const nlohmann::json& request
)
{
    return nlohmann::json::parse(runtime.process(request.dump()));
}

}  // namespace

TEST_CASE("EmbeddedPythonRuntime initializes and routes ping requests")
{
    const std::filesystem::path application_root(OPENGEOLAB_TEST_APPLICATION_ROOT);
    const std::filesystem::path project_source_root(OPENGEOLAB_PROJECT_SOURCE_DIR);

    OpenGeoLab::Python::EmbeddedPythonRuntime runtime(
        application_root,
        project_source_root / "src/app/resource/python",
        project_source_root / "plugins/python"
    );

    const auto response = processRequest(runtime, {
                                                      {"protocolVersion", "1.0"},
                                                      {"requestId", "embedded-python-test"},
                                                      {"source", "doctest"},
                                                      {"action", "system.ping"},
                                                      {"payload", nlohmann::json::object()},
                                                      {"context", nlohmann::json::object()}
                                                  });

    CHECK(response.at("ok").get<bool>());
    CHECK(response.at("action").get<std::string>() == "system.ping");
    CHECK(response.at("diagnostics").at("runtime").at("embeddedPython").get<bool>());
}
