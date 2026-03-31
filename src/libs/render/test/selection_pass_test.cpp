#include "pass/selection_pass.hpp"

#include <doctest/doctest.h>

#include <type_traits>

namespace OpenGeoLab::Render {

TEST_CASE("SelectionPass exposes const PickFbo accessor") {
    CHECK(
        (std::is_same_v<decltype(std::declval<const SelectionPass&>().pickFbo()), const PickFbo&>));
}

} // namespace OpenGeoLab::Render
