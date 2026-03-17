#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <ogl/core/IService.hpp>
#include <ogl/render/BuildFrameAction.hpp>
#include <ogl/render/RenderAction.hpp>
#include <ogl/render/RenderComponentRegistration.hpp>

#include <kangaroo/util/component_factory.hpp>

#include <vector>

namespace {

class NullRenderActionFactory final : public OGL::Render::RenderActionFactory {
public:
    auto create() -> tObjectPtr override { return {}; }
};

class RestoreBuildFrameFactory final {
public:
    ~RestoreBuildFrameFactory() {
        g_ComponentFactory.overWriteFactoryWithID<OGL::Render::BuildFrameActionFactory>(
            OGL::Render::BuildFrameAction::actionName());
    }
};

auto renderService() -> std::shared_ptr<OGL::Core::IService> {
    OGL::Render::registerRenderComponents();
    return g_ComponentFactory.getInstanceObjectWithID<OGL::Core::IServiceSingletonFactory>(
        "render");
}

} // namespace

TEST_CASE("build frame action reports staged progress and equivalent Python", "[render][actions]") {
    OGL::Render::BuildFrameAction action;
    const OGL::Core::ServiceRequest request{
        .module = "render",
        .action = OGL::Render::BuildFrameAction::actionName(),
        .param = {{"modelName", "RenderAction"}, {"bodyCount", 2}},
    };

    std::vector<double> progressValues;
    std::vector<std::string> progressMessages;
    const auto response = action.execute(
        request, [&progressValues, &progressMessages](double progress, const std::string& message) {
            progressValues.push_back(progress);
            progressMessages.push_back(message);
            return true;
        });

    REQUIRE(response.success);
    REQUIRE(progressValues.size() == 4);
    CHECK(progressValues[0] == Catch::Approx(0.2));
    CHECK(progressValues[1] == Catch::Approx(0.55));
    CHECK(progressValues[2] == Catch::Approx(0.85));
    CHECK(progressValues[3] == Catch::Approx(0.95));
    CHECK(progressMessages[0] == "Preparing geometry model for render frame...");
    CHECK(progressMessages[1] == "Building scene graph for render frame...");
    CHECK(progressMessages[2] == "Building render draw items...");
    CHECK(progressMessages[3] == "Render frame completed.");

    const auto equivalentPython = response.payload.value("equivalentPython", std::string{});
    CHECK(equivalentPython.find("OpenGeoLabPythonBridge") != std::string::npos);
    CHECK(equivalentPython.find("json.loads(") != std::string::npos);
    CHECK(equivalentPython.find("\\\"action\\\": \\\"buildFrame\\\"") != std::string::npos);
}

TEST_CASE("render service reports unsupported actions and null factories",
          "[render][registration]") {
    const auto service = renderService();

    const auto unsupportedResponse = service->processRequest(
        {.module = "render", .action = "missingRenderAction", .param = nlohmann::json::object()},
        {});
    CHECK_FALSE(unsupportedResponse.success);
    CHECK(unsupportedResponse.message ==
          "Unsupported render action. Registered actions: buildFrame.");

    RestoreBuildFrameFactory restoreFactory;
    g_ComponentFactory.overWriteFactoryWithID<NullRenderActionFactory>(
        OGL::Render::BuildFrameAction::actionName());

    const auto nullFactoryResponse =
        service->processRequest({.module = "render",
                                 .action = OGL::Render::BuildFrameAction::actionName(),
                                 .param = nlohmann::json::object()},
                                {});
    CHECK_FALSE(nullFactoryResponse.success);
    CHECK(nullFactoryResponse.message == "Render action factory resolved a null action instance.");
}
