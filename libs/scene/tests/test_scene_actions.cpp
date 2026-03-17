#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <ogl/core/IService.hpp>
#include <ogl/scene/BuildSceneAction.hpp>
#include <ogl/scene/SceneAction.hpp>
#include <ogl/scene/SceneComponentRegistration.hpp>

#include <kangaroo/util/component_factory.hpp>

#include <vector>

namespace {

class NullSceneActionFactory final : public OGL::Scene::SceneActionFactory {
public:
    auto create() -> tObjectPtr override { return {}; }
};

class RestoreBuildSceneFactory final {
public:
    ~RestoreBuildSceneFactory() {
        g_ComponentFactory.overWriteFactoryWithID<OGL::Scene::BuildSceneActionFactory>(
            OGL::Scene::BuildSceneAction::actionName());
    }
};

auto sceneService() -> std::shared_ptr<OGL::Core::IService> {
    OGL::Scene::registerSceneComponents();
    return g_ComponentFactory.getInstanceObjectWithID<OGL::Core::IServiceSingletonFactory>("scene");
}

} // namespace

TEST_CASE("build scene action reports staged progress and equivalent Python", "[scene][actions]") {
    OGL::Scene::BuildSceneAction action;
    const OGL::Core::ServiceRequest request{
        .module = "scene",
        .action = OGL::Scene::BuildSceneAction::actionName(),
        .param = {{"modelName", "SceneAction"}, {"bodyCount", 2}},
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
    REQUIRE(progressValues.size() == 3);
    CHECK(progressValues[0] == Catch::Approx(0.25));
    CHECK(progressValues[1] == Catch::Approx(0.7));
    CHECK(progressValues[2] == Catch::Approx(0.95));
    CHECK(progressMessages[0] == "Preparing geometry model for scene graph...");
    CHECK(progressMessages[1] == "Building scene graph nodes...");
    CHECK(progressMessages[2] == "Scene graph completed.");

    const auto equivalentPython = response.payload.value("equivalentPython", std::string{});
    CHECK(equivalentPython.find("OpenGeoLabPythonBridge") != std::string::npos);
    CHECK(equivalentPython.find("json.loads(") != std::string::npos);
    CHECK(equivalentPython.find("\\\"action\\\": \\\"buildScene\\\"") != std::string::npos);
}

TEST_CASE("scene service reports unsupported actions and null factories", "[scene][registration]") {
    const auto service = sceneService();

    const auto unsupportedResponse = service->processRequest(
        {.module = "scene", .action = "missingSceneAction", .param = nlohmann::json::object()}, {});
    CHECK_FALSE(unsupportedResponse.success);
    CHECK(unsupportedResponse.message ==
          "Unsupported scene action. Registered actions: buildScene.");

    RestoreBuildSceneFactory restoreFactory;
    g_ComponentFactory.overWriteFactoryWithID<NullSceneActionFactory>(
        OGL::Scene::BuildSceneAction::actionName());

    const auto nullFactoryResponse =
        service->processRequest({.module = "scene",
                                 .action = OGL::Scene::BuildSceneAction::actionName(),
                                 .param = nlohmann::json::object()},
                                {});
    CHECK_FALSE(nullFactoryResponse.success);
    CHECK(nullFactoryResponse.message == "Scene action factory resolved a null action instance.");
}
