#include "pass/render_pass_base.hpp"
#include "pass/tessellation_overlay_pass.hpp"

#include <opengeolab/render/frame_state.hpp>

#include <doctest/doctest.h>

#include <type_traits>

namespace OpenGeoLab::Render {

TEST_CASE("FrameState disables tessellation overlay by default") {
    const FrameState state;

    CHECK_FALSE(state.showTessellation);
}

TEST_CASE("TessellationOverlayPass derives from RenderPassBase") {
    CHECK((std::is_base_of_v<RenderPassBase, TessellationOverlayPass>));
}

} // namespace OpenGeoLab::Render
