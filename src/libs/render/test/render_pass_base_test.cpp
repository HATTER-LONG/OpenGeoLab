#include "pass/render_pass_base.hpp"

#include <doctest/doctest.h>

namespace OpenGeoLab::Render {
namespace {

class RecordingPass final : public RenderPassBase {
public:
    void render(const FrameState&, const GpuBufferManager&) override {}

    bool onInitialize() override {
        ++initializeCalls;
        return initializeResult;
    }

    void onCleanup() override { ++cleanupCalls; }

    bool initializeResult{true};
    int initializeCalls{0};
    int cleanupCalls{0};
};

} // namespace

TEST_CASE("RenderPassBase initializes once after success") {
    RecordingPass pass;

    pass.initialize();
    pass.initialize();

    CHECK(pass.initializeCalls == 1);
}

TEST_CASE("RenderPassBase retries initialization after failure") {
    RecordingPass pass;
    pass.initializeResult = false;

    pass.initialize();
    pass.initialize();

    CHECK(pass.initializeCalls == 2);
}

TEST_CASE("RenderPassBase cleanup runs only for initialized pass") {
    RecordingPass pass;

    pass.cleanup();
    CHECK(pass.cleanupCalls == 0);

    pass.initialize();
    pass.cleanup();
    pass.cleanup();

    CHECK(pass.cleanupCalls == 1);
}

} // namespace OpenGeoLab::Render
