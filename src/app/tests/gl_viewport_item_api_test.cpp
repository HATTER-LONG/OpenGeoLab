/**
 * @file gl_viewport_item_api_test.cpp
 * @brief API tests for GLViewportItem scene graph wiring.
 */

#include <opengeolab/app/gl_viewport_item.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <doctest/doctest.h>

#include <type_traits>

static_assert(
    std::is_same_v<decltype(&OpenGeoLab::App::GLViewportItem::setSceneGraph),
                   void (OpenGeoLab::App::GLViewportItem::*)(OpenGeoLab::Scene::SceneGraph*)>);

TEST_CASE("GLViewportItem exposes a scene graph setter") { CHECK(true); }
