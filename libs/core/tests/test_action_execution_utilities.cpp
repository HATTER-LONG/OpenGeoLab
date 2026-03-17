#include <catch2/catch_test_macros.hpp>

#include <ogl/core/ActionExecutionUtilities.hpp>

#include <vector>

TEST_CASE("action execution utilities build standard responses and equivalent Python snippets",
          "[core][actions][unit]") {
    const OGL::Core::ServiceRequest request{
        .module = "scene",
        .action = "buildScene",
        .param = {{"bodyCount", 2}, {"source", "unit-test"}},
    };

    const auto cancellationResponse =
        OGL::Core::buildCancellationResponse(request, "Scene graph construction was cancelled.");
    CHECK_FALSE(cancellationResponse.success);
    CHECK(cancellationResponse.module == "scene");
    CHECK(cancellationResponse.action == "buildScene");
    CHECK(cancellationResponse.message == "Scene graph construction was cancelled.");
    CHECK(cancellationResponse.payload.is_object());
    CHECK(cancellationResponse.payload.empty());

    const auto failureResponse =
        OGL::Core::buildFailureResponse(request, "Scene graph construction failed.");
    CHECK_FALSE(failureResponse.success);
    CHECK(failureResponse.module == "scene");
    CHECK(failureResponse.action == "buildScene");
    CHECK(failureResponse.message == "Scene graph construction failed.");
    CHECK(failureResponse.payload.is_object());
    CHECK(failureResponse.payload.empty());

    const auto equivalentPython = OGL::Core::buildEquivalentPythonSnippet(request);
    CHECK(equivalentPython.find("OpenGeoLabPythonBridge") != std::string::npos);
    CHECK(equivalentPython.find("json.loads(") != std::string::npos);
    CHECK(equivalentPython.find("\\\"module\\\": \\\"scene\\\"") != std::string::npos);
    CHECK(equivalentPython.find("\\\"action\\\": \\\"buildScene\\\"") != std::string::npos);
}

TEST_CASE("equivalent Python snippet preserves triple single quotes without raw string delimiters",
          "[core][actions][unit]") {
    const OGL::Core::ServiceRequest request{
        .module = "scene",
        .action = "buildScene",
        .param = {{"note", "Use ''' for emphasis"}},
    };

    const auto equivalentPython = OGL::Core::buildEquivalentPythonSnippet(request);

    CHECK(equivalentPython.find("json.loads(r'''") == std::string::npos);
    CHECK(equivalentPython.find("Use ''' for emphasis") != std::string::npos);
}

TEST_CASE("run progress stage supports cancellation after reporting progress",
          "[core][actions][unit]") {
    const OGL::Core::ServiceRequest request{
        .module = "render",
        .action = "buildFrame",
        .param = nlohmann::json::object(),
    };

    std::vector<double> progressValues;
    std::vector<std::string> progressMessages;
    bool stepCalled = false;
    OGL::Core::ServiceResponse earlyResponse;

    const auto callback = [&progressValues, &progressMessages](double progress,
                                                               const std::string& message) {
        progressValues.push_back(progress);
        progressMessages.push_back(message);
        return false;
    };

    const bool stageCompleted = OGL::Core::runProgressStage(
        request, callback, 0.55, "Building scene graph for render frame...",
        "Render frame construction was cancelled.", [&stepCalled]() { stepCalled = true; },
        earlyResponse);

    CHECK_FALSE(stageCompleted);
    CHECK_FALSE(stepCalled);
    REQUIRE(progressValues.size() == 1);
    CHECK(progressValues.front() == 0.55);
    CHECK(progressMessages.front() == "Building scene graph for render frame...");
    CHECK_FALSE(earlyResponse.success);
    CHECK(earlyResponse.message == "Render frame construction was cancelled.");
    CHECK(earlyResponse.payload.empty());
}

TEST_CASE("run progress stage executes work when progress continues", "[core][actions][unit]") {
    const OGL::Core::ServiceRequest request{
        .module = "selection",
        .action = "pickEntity",
        .param = nlohmann::json::object(),
    };

    bool stepCalled = false;
    OGL::Core::ServiceResponse earlyResponse;

    const bool stageCompleted = OGL::Core::runProgressStage(
        request, [](double, const std::string&) { return true; }, 0.2,
        "Building scene graph for selection...", "Selection request was cancelled.",
        [&stepCalled]() { stepCalled = true; }, earlyResponse);

    CHECK(stageCompleted);
    CHECK(stepCalled);
    CHECK(earlyResponse.module.empty());
    CHECK(earlyResponse.action.empty());
}
