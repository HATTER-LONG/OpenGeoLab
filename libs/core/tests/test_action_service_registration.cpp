#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <ogl/core/ActionServiceRegistration.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

struct TestLoggerState {
    std::vector<std::string> registrations;
    std::vector<std::string> dispatches;
    std::vector<std::string> nullFactories;
    std::vector<std::string> errors;
};

struct TestLoggerHooks {
    std::shared_ptr<TestLoggerState> state;

    void onRegistered(const std::string& moduleName, const std::string& sortedActions) const {
        state->registrations.push_back(moduleName + ":" + sortedActions);
    }

    void onDispatch(const std::string& moduleName, const std::string& actionName) const {
        state->dispatches.push_back(moduleName + ":" + actionName);
    }

    void onFactoryNull(const std::string& moduleName, const std::string& actionName) const {
        state->nullFactories.push_back(moduleName + ":" + actionName);
    }

    void onError(const std::string& moduleName,
                 const std::string& actionName,
                 const std::string& errorText) const {
        state->errors.push_back(moduleName + ":" + actionName + ":" + errorText);
    }
};

class TestAction {
public:
    virtual ~TestAction() = default;

    virtual auto execute(const OGL::Core::ServiceRequest& request,
                         const OGL::Core::ProgressCallback& progressCallback)
        -> OGL::Core::ServiceResponse = 0;
};

class EchoAction final : public TestAction {
public:
    auto execute(const OGL::Core::ServiceRequest& request, const OGL::Core::ProgressCallback&)
        -> OGL::Core::ServiceResponse override {
        return {.success = true,
                .module = request.module,
                .action = request.action,
                .message = "ok",
                .payload = nlohmann::json::object()};
    }
};

} // namespace

TEST_CASE("action service rejects duplicate supported action ids", "[core][registration][unit]") {
    const auto loggerState = std::make_shared<TestLoggerState>();
    const OGL::Core::ActionServiceRegistrationSpec<TestAction, TestLoggerHooks> spec{
        .moduleName = "demo",
        .supportedActions = {"alpha", "beta", "alpha"},
        .createActionById = [](const std::string&) { return std::make_shared<EchoAction>(); },
        .loggerHooks = {.state = loggerState},
    };

    CHECK_THROWS_WITH(
        (OGL::Core::ActionService<TestAction, TestLoggerHooks>(spec)),
        Catch::Matchers::ContainsSubstring("Duplicate action ids are not allowed: alpha"));
}

TEST_CASE(
    "action service returns sorted unsupported responses and standardized null-factory failures",
    "[core][registration][unit]") {
    const auto loggerState = std::make_shared<TestLoggerState>();
    OGL::Core::ActionService<TestAction, TestLoggerHooks> service(
        {.moduleName = "demo",
         .supportedActions = {"zeta", "alpha", "beta"},
         .createActionById = [](const std::string& actionId) -> std::shared_ptr<TestAction> {
             if(actionId == "beta") {
                 return {};
             }
             return std::make_shared<EchoAction>();
         },
         .loggerHooks = {.state = loggerState}});

    const auto unsupportedResponse = service.processRequest(
        {.module = "demo", .action = "missing", .param = nlohmann::json::object()}, {});
    CHECK_FALSE(unsupportedResponse.success);
    CHECK(unsupportedResponse.message ==
          "Unsupported demo action. Registered actions: alpha, beta, zeta.");

    const auto nullFactoryResponse = service.processRequest(
        {.module = "demo", .action = "beta", .param = nlohmann::json::object()}, {});
    CHECK_FALSE(nullFactoryResponse.success);
    CHECK(nullFactoryResponse.message == "Demo action factory resolved a null action instance.");
    CHECK(loggerState->nullFactories == std::vector<std::string>{"demo:beta"});
}
