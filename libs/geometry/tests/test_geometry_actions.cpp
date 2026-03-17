#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <ogl/core/IService.hpp>
#include <ogl/geometry/CreateBoxAction.hpp>
#include <ogl/geometry/GeometryAction.hpp>
#include <ogl/geometry/GeometryComponentRegistration.hpp>

#include <kangaroo/util/component_factory.hpp>

#include <vector>

namespace {

class NullGeometryActionFactory final : public OGL::Geometry::GeometryActionFactory {
public:
    auto create() -> tObjectPtr override { return {}; }
};

class RestoreCreateBoxFactory final {
public:
    ~RestoreCreateBoxFactory() {
        g_ComponentFactory.overWriteFactoryWithID<OGL::Geometry::CreateBoxActionFactory>(
            OGL::Geometry::CreateBoxAction::actionName());
    }
};

auto geometryService() -> std::shared_ptr<OGL::Core::IService> {
    OGL::Geometry::registerGeometryComponents();
    return g_ComponentFactory.getInstanceObjectWithID<OGL::Core::IServiceSingletonFactory>(
        "geometry");
}

} // namespace

TEST_CASE("create box action reports equivalent Python and supports mid-stage cancellation",
          "[geometry][actions]") {
    OGL::Geometry::CreateBoxAction action;
    const OGL::Core::ServiceRequest request{
        .module = "geometry",
        .action = OGL::Geometry::CreateBoxAction::actionName(),
        .param =
            {
                {"modelName", "ActionBox"},
                {"origin", {{"x", 1.0}, {"y", 2.0}, {"z", 3.0}}},
                {"dimensions", {{"x", 4.0}, {"y", 5.0}, {"z", 6.0}}},
            },
    };

    std::vector<double> progressValues;
    const auto cancelledResponse =
        action.execute(request, [&progressValues](double progress, const std::string&) {
            progressValues.push_back(progress);
            return progress < 0.3;
        });

    CHECK_FALSE(cancelledResponse.success);
    CHECK(cancelledResponse.message == "Box creation was cancelled.");
    REQUIRE(progressValues.size() == 2);
    CHECK(progressValues[0] == Catch::Approx(0.1));
    CHECK(progressValues[1] == Catch::Approx(0.3));

    const auto successResponse = action.execute(request, {});
    REQUIRE(successResponse.success);
    const auto equivalentPython = successResponse.payload.value("equivalentPython", std::string{});
    CHECK(equivalentPython.find("OpenGeoLabPythonBridge") != std::string::npos);
    CHECK(equivalentPython.find("json.loads(") != std::string::npos);
    CHECK(equivalentPython.find("\\\"action\\\": \\\"createBox\\\"") != std::string::npos);
}

TEST_CASE("geometry service reports sorted unsupported actions and null factories",
          "[geometry][registration]") {
    const auto service = geometryService();

    const auto unsupportedResponse = service->processRequest({.module = "geometry",
                                                              .action = "missingGeometryAction",
                                                              .param = nlohmann::json::object()},
                                                             {});
    CHECK_FALSE(unsupportedResponse.success);
    CHECK(unsupportedResponse.message ==
          "Unsupported geometry action. Registered actions: createBox, createCylinder, "
          "createSphere, createTorus, inspectModel.");

    RestoreCreateBoxFactory restoreFactory;
    g_ComponentFactory.overWriteFactoryWithID<NullGeometryActionFactory>(
        OGL::Geometry::CreateBoxAction::actionName());

    const auto nullFactoryResponse =
        service->processRequest({.module = "geometry",
                                 .action = OGL::Geometry::CreateBoxAction::actionName(),
                                 .param = nlohmann::json::object()},
                                {});
    CHECK_FALSE(nullFactoryResponse.success);
    CHECK(nullFactoryResponse.message ==
          "Geometry action factory resolved a null action instance.");
}
