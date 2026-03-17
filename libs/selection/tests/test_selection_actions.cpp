#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <ogl/core/IService.hpp>
#include <ogl/selection/BoxSelectAction.hpp>
#include <ogl/selection/PickEntityAction.hpp>
#include <ogl/selection/SelectionAction.hpp>
#include <ogl/selection/SelectionComponentRegistration.hpp>

#include <kangaroo/util/component_factory.hpp>

#include <vector>

namespace {

class NullSelectionActionFactory final : public OGL::Selection::SelectionActionFactory {
public:
    auto create() -> tObjectPtr override { return {}; }
};

class RestorePickEntityFactory final {
public:
    ~RestorePickEntityFactory() {
        g_ComponentFactory.overWriteFactoryWithID<OGL::Selection::PickEntityActionFactory>(
            OGL::Selection::PickEntityAction::actionName());
    }
};

auto selectionService() -> std::shared_ptr<OGL::Core::IService> {
    OGL::Selection::registerSelectionComponents();
    return g_ComponentFactory.getInstanceObjectWithID<OGL::Core::IServiceSingletonFactory>(
        "selection");
}

} // namespace

TEST_CASE("box select action reports staged progress and equivalent Python",
          "[selection][actions]") {
    OGL::Selection::BoxSelectAction action;
    const OGL::Core::ServiceRequest request{
        .module = "selection",
        .action = OGL::Selection::BoxSelectAction::actionName(),
        .param =
            {
                {"modelName", "SelectionAction"},
                {"bodyCount", 3},
                {"selectionCount", 2},
                {"startIndex", 1},
            },
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
    CHECK(progressMessages[0] == "Building scene graph for selection...");
    CHECK(progressMessages[1] == "Building render frame for selection...");
    CHECK(progressMessages[2] == "Evaluating selection hits...");
    CHECK(progressMessages[3] == "Box selection completed.");

    const auto equivalentPython = response.payload.value("equivalentPython", std::string{});
    CHECK(equivalentPython.find("OpenGeoLabPythonBridge") != std::string::npos);
    CHECK(equivalentPython.find("json.loads(") != std::string::npos);
    CHECK(equivalentPython.find("\\\"action\\\": \\\"boxSelect\\\"") != std::string::npos);
}

TEST_CASE("selection service reports sorted unsupported actions and null factories",
          "[selection][registration]") {
    const auto service = selectionService();

    const auto unsupportedResponse = service->processRequest({.module = "selection",
                                                              .action = "missingSelectionAction",
                                                              .param = nlohmann::json::object()},
                                                             {});
    CHECK_FALSE(unsupportedResponse.success);
    CHECK(unsupportedResponse.message ==
          "Unsupported selection action. Registered actions: boxSelect, pickEntity.");

    RestorePickEntityFactory restoreFactory;
    g_ComponentFactory.overWriteFactoryWithID<NullSelectionActionFactory>(
        OGL::Selection::PickEntityAction::actionName());

    const auto nullFactoryResponse =
        service->processRequest({.module = "selection",
                                 .action = OGL::Selection::PickEntityAction::actionName(),
                                 .param = nlohmann::json::object()},
                                {});
    CHECK_FALSE(nullFactoryResponse.success);
    CHECK(nullFactoryResponse.message ==
          "Selection action factory resolved a null action instance.");
}
