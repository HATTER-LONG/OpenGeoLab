/**
 * @file model_reader_import_logging.cpp
 * @brief Tests for ModelReader (import) with multi-level logging output
 */

#include <catch2/catch_test_macros.hpp>

#include <app/service.hpp>
#include <io/model_reader.hpp>
#include <util/logger.hpp>

#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace {

class TestProgressReporter final : public OpenGeoLab::App::IProgressReporter {
public:
    void reportProgress(double progress, const std::string& message) override {
        (void)progress;
        messages.push_back(message);
        ++progressCalls;
    }

    void reportError(const std::string& error_message) override { errors.push_back(error_message); }

    bool isCancelled() const override { return cancelled.load(); }

    void cancel() { cancelled.store(true); }

    std::vector<std::string> messages;
    std::vector<std::string> errors;
    std::atomic<bool> cancelled{false};
    std::atomic<int> progressCalls{0};
};

} // namespace

TEST_CASE("Import (ModelReader) - emits logs across levels") {
    auto logger = OpenGeoLab::getLogger();
    REQUIRE(logger);

    logger->set_level(spdlog::level::trace);

    auto reporter = std::make_shared<TestProgressReporter>();

    OpenGeoLab::IO::ModelReader reader;
    nlohmann::json params;
    params["fast"] = true;
    params["max_steps"] = 3;

    const auto result = reader.processRequest("model_reader", params, reporter);

    REQUIRE(result.contains("module_name"));
    CHECK(result["module_name"] == "model_reader");
    CHECK(reporter->progressCalls.load() > 0);

    // Print all levels as part of the import test.
    LOG_TRACE("import test: trace");
    LOG_DEBUG("import test: debug");
    LOG_INFO("import test: info");
    LOG_WARN("import test: warn");
    LOG_ERROR("import test: error");
    LOG_CRITICAL("import test: critical");
}
