/**
 * @file pass_manager_test.cpp
 * @brief Unit tests for PassManager dispatch logic.
 */
#include <opengeolab/render/pass_manager.hpp>

#include <doctest/doctest.h>

#include <memory>
#include <string>
#include <vector>

using OpenGeoLab::Render::IRenderPass;
using OpenGeoLab::Render::PassManager;
using OpenGeoLab::Render::RenderContext;

namespace {

class MockPass final : public IRenderPass {
public:
    explicit MockPass(std::string tag, std::vector<std::string>& log)
        : tag_(std::move(tag)), log_(log) {}

    void setup(int /*width*/, int /*height*/) override { log_.push_back(tag_ + ":setup"); }
    void execute(const RenderContext& /*ctx*/) override { log_.push_back(tag_ + ":execute"); }
    void teardown() override { log_.push_back(tag_ + ":teardown"); }

private:
    std::string tag_;
    std::vector<std::string>& log_;
};

} // namespace

TEST_CASE("PassManager") {
    PassManager mgr;
    std::vector<std::string> log;

    SUBCASE("registerPass increases count") {
        CHECK(mgr.passCount() == 0);
        mgr.registerPass("A", std::make_unique<MockPass>("A", log), 10);
        CHECK(mgr.passCount() == 1);
    }

    SUBCASE("executeAll calls passes in priority order") {
        mgr.registerPass("C", std::make_unique<MockPass>("C", log), 300);
        mgr.registerPass("A", std::make_unique<MockPass>("A", log), 100);
        mgr.registerPass("B", std::make_unique<MockPass>("B", log), 200);

        RenderContext ctx;
        mgr.executeAll(ctx);

        REQUIRE(log.size() == 3);
        CHECK(log[0] == "A:execute");
        CHECK(log[1] == "B:execute");
        CHECK(log[2] == "C:execute");
    }

    SUBCASE("disabled pass is skipped during executeAll") {
        mgr.registerPass("A", std::make_unique<MockPass>("A", log), 100);
        mgr.registerPass("B", std::make_unique<MockPass>("B", log), 200);
        mgr.setPassEnabled("A", false);

        RenderContext ctx;
        mgr.executeAll(ctx);

        REQUIRE(log.size() == 1);
        CHECK(log[0] == "B:execute");
    }

    SUBCASE("setPassEnabled toggles correctly") {
        mgr.registerPass("X", std::make_unique<MockPass>("X", log), 1);
        CHECK(mgr.isPassEnabled("X"));
        mgr.setPassEnabled("X", false);
        CHECK_FALSE(mgr.isPassEnabled("X"));
        mgr.setPassEnabled("X", true);
        CHECK(mgr.isPassEnabled("X"));
    }

    SUBCASE("setupAll calls setup on all passes") {
        mgr.registerPass("A", std::make_unique<MockPass>("A", log), 1);
        mgr.registerPass("B", std::make_unique<MockPass>("B", log), 2);
        mgr.setupAll(800, 600);
        REQUIRE(log.size() == 2);
        CHECK(log[0] == "A:setup");
        CHECK(log[1] == "B:setup");
    }

    SUBCASE("teardownAll calls teardown on all passes") {
        mgr.registerPass("A", std::make_unique<MockPass>("A", log), 1);
        mgr.registerPass("B", std::make_unique<MockPass>("B", log), 2);
        mgr.teardownAll();
        REQUIRE(log.size() == 2);
        CHECK(log[0] == "A:teardown");
        CHECK(log[1] == "B:teardown");
    }

    SUBCASE("duplicate priority is allowed") {
        mgr.registerPass("A", std::make_unique<MockPass>("A", log), 100);
        mgr.registerPass("B", std::make_unique<MockPass>("B", log), 100);
        CHECK(mgr.passCount() == 2);
        RenderContext ctx;
        mgr.executeAll(ctx);
        CHECK(log.size() == 2);
    }
}