# Viewport Actions Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Migrate CameraState to scene layer, add viewport actions (camera control + pick area) accessible from Python scripting.

**Architecture:** Single-copy ViewportState in scene layer (mutex-protected) holds CameraState, pending pick-area requests, and version tracking. Four new actions registered in SceneModule. App layer reads/writes camera through ViewportState instead of local copy.

**Tech Stack:** C++20, doctest, CMake/Ninja, Qt 6, glm, nlohmann::json, Kangaroo signals

**Spec:** `docs/superpowers/specs/2026-04-02-viewport-actions-design.md`

---

## Build & Test

Configure (only if `build/` does not exist):
```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

Build full project:
```
cmake --build build --config RelWithDebInfo --parallel 8
```

Build scene library only:
```
cmake --build build --config RelWithDebInfo --target opengeolab_scene --parallel 8
```

Run all tests:
```
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```

Run scene tests only:
```
ctest --test-dir build -C RelWithDebInfo -R scene --output-on-failure
```

---

### Task 1: Create ViewPreset enum in scene layer

**Files:**
- Create: `src/libs/scene/include/opengeolab/scene/view_preset.hpp`

- [ ] **Step 1: Create view_preset.hpp**

```cpp
/**
 * @file view_preset.hpp
 * @brief Standard camera view presets
 */

#pragma once

#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

/// @brief 7 standard camera view presets.
enum class ViewPreset { Front, Back, Top, Bottom, Left, Right, Isometric };

} // namespace OpenGeoLab::Scene
```

No test needed — enum with no logic.

- [ ] **Step 2: Commit**

```
git add src/libs/scene/include/opengeolab/scene/view_preset.hpp
git commit -m "feat(scene): add ViewPreset enum for camera presets"
```

---

### Task 2: Migrate CameraState from app to scene layer

**Files:**
- Create: `src/libs/scene/include/opengeolab/scene/camera_state.hpp`
- Create: `src/libs/scene/src/camera_state.cpp`
- Create: `src/libs/scene/test/camera_state_test.cpp`
- Modify: `src/libs/scene/CMakeLists.txt`

This is a migration: copy existing code, change namespace from `OpenGeoLab::App` to `OpenGeoLab::Scene`, add export macro. The existing app files will be removed in Task 8.

- [ ] **Step 1: Create scene camera_state.hpp**

```cpp
/**
 * @file camera_state.hpp
 * @brief Cartesian camera state with orthographic projection
 */

#pragma once

#include <opengeolab/scene/bounding_box3d.hpp>
#include <opengeolab/scene/scene_export.hpp>

#include <glm/glm.hpp>

namespace OpenGeoLab::Scene {

/**
 * @brief Cartesian camera model for orthographic rendering
 *
 * Stores position/target/up vectors and symmetric near/far clipping.
 * Provides view and orthographic projection matrices.
 */
struct OPENGEOLAB_SCENE_EXPORT CameraState {
    glm::vec3 position{0.0F, 0.0F, 50.0F}; /**< Eye position */
    glm::vec3 target{0.0F, 0.0F, 0.0F};    /**< Look-at target */
    glm::vec3 up{0.0F, 1.0F, 0.0F};        /**< Up direction */
    float nearPlane{-500.0F};              /**< Near clipping plane */
    float farPlane{500.0F};                /**< Far clipping plane */

    /** @brief Compute the view matrix via glm::lookAt. */
    [[nodiscard]] glm::mat4 viewMatrix() const;

    /**
     * @brief Compute orthographic projection matrix.
     * @param aspect Viewport width / height.
     */
    [[nodiscard]] glm::mat4 projMatrix(float aspect) const;

    /** @brief Get eye position (alias for position). */
    [[nodiscard]] glm::vec3 eyePosition() const { return position; }

    /** @brief Distance from eye to target. */
    [[nodiscard]] float distance() const;

    /** @brief Recalculate near/far planes as ±10× distance. */
    void updateClipping();

    /** @brief Reset to default values. */
    void reset();

    /**
     * @brief Position camera to view the entire bounding box.
     * @param bounds Scene bounds to frame.
     */
    void fitToBoundingBox(const BoundingBox3D& bounds);
};

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 2: Create scene camera_state.cpp**

```cpp
/**
 * @file camera_state.cpp
 * @brief CameraState implementation — view/projection matrix computation and camera framing
 */

#include <opengeolab/scene/camera_state.hpp>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <algorithm>

namespace OpenGeoLab::Scene {

glm::mat4 CameraState::viewMatrix() const { return glm::lookAt(position, target, up); }

float CameraState::distance() const { return glm::length(position - target); }

glm::mat4 CameraState::projMatrix(float aspect) const {
    const float camera_distance = distance();
    const float half_height = camera_distance * 0.5F;
    const float half_width = half_height * aspect;
    return glm::ortho(-half_width, half_width, -half_height, half_height, nearPlane, farPlane);
}

void CameraState::updateClipping() {
    const float camera_distance = std::max(distance(), 1.0e-4F);
    const float half_range = camera_distance * 10.0F;
    nearPlane = -half_range;
    farPlane = half_range;
}

void CameraState::reset() {
    position = {0.0F, 0.0F, 50.0F};
    target = {0.0F, 0.0F, 0.0F};
    up = {0.0F, 1.0F, 0.0F};
    updateClipping();
}

void CameraState::fitToBoundingBox(const BoundingBox3D& bounds) {
    if(!bounds.isValid()) {
        reset();
        return;
    }

    target = bounds.center();
    const float diagonal = bounds.diagonal();
    const float fit_distance = diagonal * 1.5F;
    position = target + glm::vec3{0.0F, 0.0F, fit_distance};
    updateClipping();
}

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 3: Create scene camera_state_test.cpp**

Copy from `src/app/test/camera_state_test.cpp`, change namespace to `OpenGeoLab::Scene::Tests` and include to `<opengeolab/scene/camera_state.hpp>`:

```cpp
/**
 * @file camera_state_test.cpp
 * @brief Unit tests for Scene::CameraState
 */

#include <opengeolab/scene/camera_state.hpp>

#include <doctest/doctest.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace OpenGeoLab::Scene::Tests {

namespace {

void checkVec3(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x));
    CHECK(actual.y == doctest::Approx(expected.y));
    CHECK(actual.z == doctest::Approx(expected.z));
}

void checkMat4(const glm::mat4& actual, const glm::mat4& expected) {
    for(int column = 0; column < 4; ++column) {
        for(int row = 0; row < 4; ++row) {
            CHECK(actual[column][row] == doctest::Approx(expected[column][row]));
        }
    }
}

} // namespace

TEST_CASE("Scene::CameraState view and projection matrices") {
    CameraState camera;
    camera.position = {3.0F, 4.0F, 20.0F};
    camera.target = {1.0F, 2.0F, 5.0F};
    camera.up = {0.0F, 1.0F, 0.0F};
    camera.nearPlane = -100.0F;
    camera.farPlane = 100.0F;

    const float aspect = 2.0F;
    const float distance = glm::length(camera.position - camera.target);
    const float half_height = distance * 0.5F;
    const float half_width = half_height * aspect;

    checkMat4(camera.viewMatrix(), glm::lookAt(camera.position, camera.target, camera.up));
    checkMat4(camera.projMatrix(aspect),
              glm::ortho(-half_width, half_width, -half_height, half_height, camera.nearPlane,
                         camera.farPlane));
    CHECK(camera.eyePosition().x == doctest::Approx(camera.position.x));
    CHECK(camera.distance() == doctest::Approx(distance));
}

TEST_CASE("Scene::CameraState reset and updateClipping") {
    CameraState camera;
    camera.position = {5.0F, -2.0F, 6.0F};
    camera.target = {1.0F, -2.0F, 1.0F};

    camera.updateClipping();

    const float expected_half_range = glm::length(camera.position - camera.target) * 10.0F;
    CHECK(camera.nearPlane == doctest::Approx(-expected_half_range));
    CHECK(camera.farPlane == doctest::Approx(expected_half_range));

    camera.reset();
    checkVec3(camera.position, glm::vec3{0.0F, 0.0F, 50.0F});
    checkVec3(camera.target, glm::vec3{0.0F, 0.0F, 0.0F});
}

TEST_CASE("Scene::CameraState fitToBoundingBox") {
    CameraState camera;
    BoundingBox3D bounds;
    bounds.expand(glm::vec3{-2.0F, -1.0F, 3.0F});
    bounds.expand(glm::vec3{6.0F, 5.0F, 9.0F});

    camera.fitToBoundingBox(bounds);

    const float fit_distance = bounds.diagonal() * 1.5F;
    checkVec3(camera.target, glm::vec3{2.0F, 2.0F, 6.0F});
    checkVec3(camera.position, glm::vec3{2.0F, 2.0F, 6.0F + fit_distance});

    camera.fitToBoundingBox(BoundingBox3D{});
    checkVec3(camera.position, glm::vec3{0.0F, 0.0F, 50.0F});
}

} // namespace OpenGeoLab::Scene::Tests
```

- [ ] **Step 4: Add to scene CMakeLists.txt**

In `src/libs/scene/CMakeLists.txt`:

Add to `scene_public_headers` list (after `include/opengeolab/scene/bounding_box3d.hpp`):
```
    include/opengeolab/scene/camera_state.hpp
```

Add to `scene_public_headers` list (before `include/opengeolab/scene/topology_index.hpp`):
```
    include/opengeolab/scene/view_preset.hpp
```

Add to `scene_sources` list (after `src/bounding_box3d.cpp`):
```
    src/camera_state.cpp
```

Add test block (after the `opengeolab_selection_actions_test` block, before `endif ()`):
```cmake
    opengeolab_add_doctest_test(
        opengeolab_scene_camera_state_test
        SOURCES test/camera_state_test.cpp
        LINKS OpenGeoLab::Scene)
```

- [ ] **Step 5: Build scene library and run test**

Run: `cmake --build build --config RelWithDebInfo --target opengeolab_scene --parallel 8`
Expected: Build succeeds.

Run: `ctest --test-dir build -C RelWithDebInfo -R scene_camera_state --output-on-failure`
Expected: All tests PASS (migration preserves behavior).

- [ ] **Step 6: Commit**

```
git add src/libs/scene/include/opengeolab/scene/camera_state.hpp \
        src/libs/scene/src/camera_state.cpp \
        src/libs/scene/test/camera_state_test.cpp \
        src/libs/scene/CMakeLists.txt
git commit -m "feat(scene): migrate CameraState from app to scene layer"
```

---

### Task 3: Create ViewportState with tests

**Files:**
- Create: `src/libs/scene/include/opengeolab/scene/viewport_state.hpp`
- Create: `src/libs/scene/src/viewport_state.cpp`
- Create: `src/libs/scene/test/viewport_state_test.cpp`
- Modify: `src/libs/scene/CMakeLists.txt`

- [ ] **Step 1: Write failing test — viewport_state_test.cpp**

```cpp
/**
 * @file viewport_state_test.cpp
 * @brief Unit tests for ViewportState
 */

#include <opengeolab/scene/viewport_state.hpp>

#include <doctest/doctest.h>

using OpenGeoLab::Core::PickAction;
using OpenGeoLab::Scene::BoundingBox3D;
using OpenGeoLab::Scene::CameraState;
using OpenGeoLab::Scene::PendingPickArea;
using OpenGeoLab::Scene::PickAreaCoordType;
using OpenGeoLab::Scene::ViewportState;
using OpenGeoLab::Scene::ViewPreset;

TEST_SUITE("ViewportState") {

    TEST_CASE("default camera matches CameraState reset") {
        ViewportState vps;
        const auto cam = vps.camera();
        CHECK(cam.position.x == doctest::Approx(0.0F));
        CHECK(cam.position.y == doctest::Approx(0.0F));
        CHECK(cam.position.z == doctest::Approx(50.0F));
        CHECK(cam.target == glm::vec3(0.0F));
    }

    TEST_CASE("setCamera updates state and bumps version") {
        ViewportState vps;
        const auto v0 = vps.cameraVersion();

        CameraState state;
        state.position = {10.0F, 20.0F, 30.0F};
        state.target = {1.0F, 2.0F, 3.0F};
        vps.setCamera(state);

        CHECK(vps.cameraVersion() > v0);
        const auto cam = vps.camera();
        CHECK(cam.position.x == doctest::Approx(10.0F));
        CHECK(cam.position.y == doctest::Approx(20.0F));
        CHECK(cam.position.z == doctest::Approx(30.0F));
        CHECK(cam.target.y == doctest::Approx(2.0F));
    }

    TEST_CASE("setViewPreset Front places camera on +Z axis") {
        ViewportState vps;
        vps.setViewPreset(ViewPreset::Front);
        const auto cam = vps.camera();
        CHECK(cam.position.z > cam.target.z);
        CHECK(cam.position.x == doctest::Approx(cam.target.x));
        CHECK(cam.position.y == doctest::Approx(cam.target.y));
    }

    TEST_CASE("setViewPreset Top places camera on +Y axis with -Z up") {
        ViewportState vps;
        vps.setViewPreset(ViewPreset::Top);
        const auto cam = vps.camera();
        CHECK(cam.position.y > cam.target.y);
        CHECK(cam.up.z == doctest::Approx(-1.0F));
    }

    TEST_CASE("setViewPreset Isometric uses diagonal direction") {
        ViewportState vps;
        vps.setViewPreset(ViewPreset::Isometric);
        const auto cam = vps.camera();
        const auto offset = cam.position - cam.target;
        CHECK(offset.x == doctest::Approx(offset.y));
        CHECK(offset.y == doctest::Approx(offset.z));
    }

    TEST_CASE("fitToBounds frames valid bounds") {
        ViewportState vps;
        BoundingBox3D bounds;
        bounds.expand(glm::vec3{-5.0F, -5.0F, -5.0F});
        bounds.expand(glm::vec3{5.0F, 5.0F, 5.0F});

        vps.fitToBounds(bounds);

        const auto cam = vps.camera();
        CHECK(cam.target.x == doctest::Approx(0.0F));
        CHECK(cam.target.y == doctest::Approx(0.0F));
        CHECK(cam.target.z == doctest::Approx(0.0F));

        const float expected_distance = bounds.diagonal() * 1.5F;
        CHECK(cam.position.z == doctest::Approx(expected_distance));
    }

    TEST_CASE("fitToBounds with invalid bounds resets camera") {
        ViewportState vps;
        CameraState custom;
        custom.position = {99.0F, 99.0F, 99.0F};
        vps.setCamera(custom);

        vps.fitToBounds(BoundingBox3D{});

        const auto cam = vps.camera();
        CHECK(cam.position.z == doctest::Approx(50.0F));
    }

    TEST_CASE("requestPickArea and consumePickArea round-trip") {
        ViewportState vps;
        CHECK_FALSE(vps.consumePickArea().has_value());

        const PendingPickArea req{
            0.1F, 0.2F, 0.8F, 0.9F, PickAreaCoordType::Normalized, PickAction::Add};
        vps.requestPickArea(req);

        const auto consumed = vps.consumePickArea();
        REQUIRE(consumed.has_value());
        CHECK(consumed->x0 == doctest::Approx(0.1F));
        CHECK(consumed->y0 == doctest::Approx(0.2F));
        CHECK(consumed->x1 == doctest::Approx(0.8F));
        CHECK(consumed->y1 == doctest::Approx(0.9F));
        CHECK(consumed->coordType == PickAreaCoordType::Normalized);
        CHECK(consumed->action == PickAction::Add);

        CHECK_FALSE(vps.consumePickArea().has_value());
    }

    TEST_CASE("cameraChanged signal fires on setCamera") {
        ViewportState vps;
        int count = 0;
        auto conn = vps.cameraChanged.connect([&]() { ++count; });

        CameraState state;
        state.position = {1.0F, 2.0F, 3.0F};
        vps.setCamera(state);
        CHECK(count == 1);

        vps.setViewPreset(ViewPreset::Front);
        CHECK(count == 2);

        BoundingBox3D bounds;
        bounds.expand(glm::vec3{-1.0F, -1.0F, -1.0F});
        bounds.expand(glm::vec3{1.0F, 1.0F, 1.0F});
        vps.fitToBounds(bounds);
        CHECK(count == 3);
    }

    TEST_CASE("pickAreaRequested signal fires on requestPickArea") {
        ViewportState vps;
        int count = 0;
        auto conn = vps.pickAreaRequested.connect([&]() { ++count; });

        vps.requestPickArea({0.0F, 0.0F, 1.0F, 1.0F, PickAreaCoordType::Normalized, PickAction::Add});
        CHECK(count == 1);
    }

} // TEST_SUITE
```

- [ ] **Step 2: Create viewport_state.hpp**

```cpp
/**
 * @file viewport_state.hpp
 * @brief ViewportState — mutex-protected camera + pending pick area
 *
 * Single source of truth for camera state. Shared by GUI thread
 * (TrackballController), worker threads (actions), and render thread
 * (synchronize reads camera snapshot).
 */

#pragma once

#include <opengeolab/core/pick_action.hpp>
#include <opengeolab/scene/bounding_box3d.hpp>
#include <opengeolab/scene/camera_state.hpp>
#include <opengeolab/scene/scene_export.hpp>
#include <opengeolab/scene/view_preset.hpp>

#include <kangaroo/util/signal.hpp>

#include <atomic>
#include <mutex>
#include <optional>

namespace OpenGeoLab::Scene {

/// @brief Coordinate type for pick area requests.
enum class PickAreaCoordType { Normalized, Pixel };

/// @brief Pending pick area request (async, consumed by renderer).
struct PendingPickArea {
    float x0{0.0F}; ///< Left/start X
    float y0{0.0F}; ///< Top/start Y
    float x1{0.0F}; ///< Right/end X
    float y1{0.0F}; ///< Bottom/end Y
    PickAreaCoordType coordType{PickAreaCoordType::Normalized};
    Core::PickAction action{Core::PickAction::Add};
};

/**
 * @brief Thread-safe viewport state: camera + pending pick requests.
 *
 * All accessors acquire m_mutex. Signals are emitted after releasing
 * the lock to avoid deadlock in signal handlers.
 */
class OPENGEOLAB_SCENE_EXPORT ViewportState final {
public:
    ViewportState();
    ~ViewportState();

    /// @brief Read current camera state (lock → copy → unlock).
    [[nodiscard]] CameraState camera() const;

    /// @brief Replace camera state (lock → set → unlock, bump version).
    void setCamera(const CameraState& state);

    /// @brief Monotonic camera version counter.
    [[nodiscard]] uint64_t cameraVersion() const noexcept;

    /// @brief Frame camera to view the given bounding box.
    void fitToBounds(const BoundingBox3D& bounds);

    /// @brief Apply a standard camera view preset.
    void setViewPreset(ViewPreset preset);

    /// @brief Queue an async pick area request for the renderer.
    void requestPickArea(const PendingPickArea& request);

    /// @brief Consume and return the pending pick area (if any).
    [[nodiscard]] std::optional<PendingPickArea> consumePickArea();

    Kangaroo::Util::Signal<> cameraChanged;     ///< Fired after camera mutation
    Kangaroo::Util::Signal<> pickAreaRequested;  ///< Fired after pick area queued

private:
    mutable std::mutex m_mutex;
    CameraState m_camera;
    std::atomic<uint64_t> m_cameraVersion{0};
    std::optional<PendingPickArea> m_pendingPickArea;
};

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 3: Create viewport_state.cpp**

```cpp
/**
 * @file viewport_state.cpp
 * @brief ViewportState implementation
 */

#include <opengeolab/scene/viewport_state.hpp>

#include <glm/glm.hpp>

#include <cmath>

namespace OpenGeoLab::Scene {

ViewportState::ViewportState() { m_camera.reset(); }
ViewportState::~ViewportState() = default;

CameraState ViewportState::camera() const {
    std::lock_guard lock(m_mutex);
    return m_camera;
}

void ViewportState::setCamera(const CameraState& state) {
    {
        std::lock_guard lock(m_mutex);
        m_camera = state;
        m_cameraVersion.fetch_add(1, std::memory_order_relaxed);
    }
    cameraChanged.emit();
}

uint64_t ViewportState::cameraVersion() const noexcept {
    return m_cameraVersion.load(std::memory_order_relaxed);
}

void ViewportState::fitToBounds(const BoundingBox3D& bounds) {
    {
        std::lock_guard lock(m_mutex);
        m_camera.fitToBoundingBox(bounds);
        m_cameraVersion.fetch_add(1, std::memory_order_relaxed);
    }
    cameraChanged.emit();
}

void ViewportState::setViewPreset(ViewPreset preset) {
    {
        std::lock_guard lock(m_mutex);
        const float dist = m_camera.distance();
        glm::vec3 direction;
        glm::vec3 up_direction{0.0F, 1.0F, 0.0F};

        switch(preset) {
        case ViewPreset::Front:
            direction = {0.0F, 0.0F, 1.0F};
            break;
        case ViewPreset::Back:
            direction = {0.0F, 0.0F, -1.0F};
            break;
        case ViewPreset::Top:
            direction = {0.0F, 1.0F, 0.0F};
            up_direction = {0.0F, 0.0F, -1.0F};
            break;
        case ViewPreset::Bottom:
            direction = {0.0F, -1.0F, 0.0F};
            up_direction = {0.0F, 0.0F, 1.0F};
            break;
        case ViewPreset::Left:
            direction = {-1.0F, 0.0F, 0.0F};
            break;
        case ViewPreset::Right:
            direction = {1.0F, 0.0F, 0.0F};
            break;
        case ViewPreset::Isometric:
            direction = glm::normalize(glm::vec3{1.0F, 1.0F, 1.0F});
            break;
        }

        m_camera.position = m_camera.target + direction * dist;
        m_camera.up = up_direction;
        m_camera.updateClipping();
        m_cameraVersion.fetch_add(1, std::memory_order_relaxed);
    }
    cameraChanged.emit();
}

void ViewportState::requestPickArea(const PendingPickArea& request) {
    {
        std::lock_guard lock(m_mutex);
        m_pendingPickArea = request;
    }
    pickAreaRequested.emit();
}

std::optional<PendingPickArea> ViewportState::consumePickArea() {
    std::lock_guard lock(m_mutex);
    auto result = std::move(m_pendingPickArea);
    m_pendingPickArea.reset();
    return result;
}

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 4: Add to scene CMakeLists.txt**

Add to `scene_public_headers` (after `camera_state.hpp`):
```
    include/opengeolab/scene/viewport_state.hpp
```

Add to `scene_sources` (after `camera_state.cpp`):
```
    src/viewport_state.cpp
```

Add test block (after `opengeolab_scene_camera_state_test`):
```cmake
    opengeolab_add_doctest_test(
        opengeolab_viewport_state_test
        SOURCES test/viewport_state_test.cpp
        LINKS OpenGeoLab::Scene)
```

- [ ] **Step 5: Build and run test (RED → GREEN)**

Run: `cmake --build build --config RelWithDebInfo --target opengeolab_scene --parallel 8`
Run: `ctest --test-dir build -C RelWithDebInfo -R viewport_state --output-on-failure`
Expected: All tests PASS.

- [ ] **Step 6: Commit**

```
git add src/libs/scene/include/opengeolab/scene/viewport_state.hpp \
        src/libs/scene/src/viewport_state.cpp \
        src/libs/scene/test/viewport_state_test.cpp \
        src/libs/scene/CMakeLists.txt
git commit -m "feat(scene): add ViewportState with mutex-protected camera and pick area"
```

---

### Task 4: Integrate ViewportState into SceneGraph

**Files:**
- Modify: `src/libs/scene/include/opengeolab/scene/scene_graph.hpp`

- [ ] **Step 1: Add ViewportState to SceneGraph header**

In `src/libs/scene/include/opengeolab/scene/scene_graph.hpp`:

Add include after `#include <opengeolab/scene/selection_state.hpp>`:
```cpp
#include <opengeolab/scene/viewport_state.hpp>
```

Add public accessors after `labelManager()` accessors (after line 163):
```cpp
    /** @brief Viewport camera and pick state. */
    [[nodiscard]] ViewportState& viewportState() { return m_viewportState; }
    [[nodiscard]] const ViewportState& viewportState() const { return m_viewportState; }
```

Add private member after `LabelManager m_labelManager;` (after line 200):
```cpp
    ViewportState m_viewportState;
```

- [ ] **Step 2: Build scene library**

Run: `cmake --build build --config RelWithDebInfo --target opengeolab_scene --parallel 8`
Expected: Build succeeds.

- [ ] **Step 3: Commit**

```
git add src/libs/scene/include/opengeolab/scene/scene_graph.hpp
git commit -m "feat(scene): add ViewportState to SceneGraph"
```

---

### Task 5: Create camera actions with tests

**Files:**
- Create: `src/libs/scene/include/opengeolab/scene/fit_to_scene_action.hpp`
- Create: `src/libs/scene/src/fit_to_scene_action.cpp`
- Create: `src/libs/scene/include/opengeolab/scene/set_view_preset_action.hpp`
- Create: `src/libs/scene/src/set_view_preset_action.cpp`
- Create: `src/libs/scene/include/opengeolab/scene/set_camera_action.hpp`
- Create: `src/libs/scene/src/set_camera_action.cpp`
- Create: `src/libs/scene/include/opengeolab/scene/pick_area_action.hpp`
- Create: `src/libs/scene/src/pick_area_action.cpp`
- Create: `src/libs/scene/test/viewport_actions_test.cpp`
- Modify: `src/libs/scene/CMakeLists.txt`

- [ ] **Step 1: Write failing test — viewport_actions_test.cpp**

```cpp
/**
 * @file viewport_actions_test.cpp
 * @brief Tests for viewport camera and pick area actions.
 */

#include <opengeolab/scene/fit_to_scene_action.hpp>
#include <opengeolab/scene/pick_area_action.hpp>
#include <opengeolab/scene/scene_graph.hpp>
#include <opengeolab/scene/set_camera_action.hpp>
#include <opengeolab/scene/set_view_preset_action.hpp>

#include <doctest/doctest.h>

using OpenGeoLab::Scene::FitToSceneAction;
using OpenGeoLab::Scene::PickAreaAction;
using OpenGeoLab::Scene::SceneGraph;
using OpenGeoLab::Scene::SetCameraAction;
using OpenGeoLab::Scene::SetViewPresetAction;

TEST_SUITE("ViewportActions") {

    TEST_CASE("FitToSceneAction resets camera when scene is empty") {
        SceneGraph graph;
        FitToSceneAction action(graph);

        const auto result = action.execute({}, nullptr);
        CHECK(result["ok"] == true);

        const auto cam = graph.viewportState().camera();
        CHECK(cam.position.z == doctest::Approx(50.0F));
    }

    TEST_CASE("SetViewPresetAction sets Front view") {
        SceneGraph graph;
        auto& vps = graph.viewportState();
        SetViewPresetAction action(vps);

        const auto result = action.execute({{"preset", "Front"}}, nullptr);
        CHECK(result["ok"] == true);

        const auto cam = vps.camera();
        CHECK(cam.position.z > cam.target.z);
        CHECK(cam.position.x == doctest::Approx(cam.target.x));
    }

    TEST_CASE("SetViewPresetAction sets Top view") {
        SceneGraph graph;
        auto& vps = graph.viewportState();
        SetViewPresetAction action(vps);

        const auto result = action.execute({{"preset", "Top"}}, nullptr);
        CHECK(result["ok"] == true);

        const auto cam = vps.camera();
        CHECK(cam.position.y > cam.target.y);
        CHECK(cam.up.z == doctest::Approx(-1.0F));
    }

    TEST_CASE("SetViewPresetAction rejects invalid preset") {
        SceneGraph graph;
        SetViewPresetAction action(graph.viewportState());

        const auto result = action.execute({{"preset", "InvalidPreset"}}, nullptr);
        CHECK(result["ok"] == false);
    }

    TEST_CASE("SetCameraAction sets position, target, up") {
        SceneGraph graph;
        auto& vps = graph.viewportState();
        SetCameraAction action(vps);

        const auto result = action.execute(
            {{"position", {10.0, 20.0, 30.0}},
             {"target", {1.0, 2.0, 3.0}},
             {"up", {0.0, 1.0, 0.0}}},
            nullptr);

        CHECK(result["ok"] == true);
        const auto cam = vps.camera();
        CHECK(cam.position.x == doctest::Approx(10.0F));
        CHECK(cam.position.y == doctest::Approx(20.0F));
        CHECK(cam.position.z == doctest::Approx(30.0F));
        CHECK(cam.target.x == doctest::Approx(1.0F));
        CHECK(cam.target.y == doctest::Approx(2.0F));
        CHECK(cam.target.z == doctest::Approx(3.0F));
    }

    TEST_CASE("SetCameraAction rejects missing position") {
        SceneGraph graph;
        SetCameraAction action(graph.viewportState());

        const auto result = action.execute(
            {{"target", {0.0, 0.0, 0.0}}, {"up", {0.0, 1.0, 0.0}}}, nullptr);
        CHECK(result["ok"] == false);
    }

    TEST_CASE("PickAreaAction stores pending pick with normalized coords") {
        SceneGraph graph;
        auto& vps = graph.viewportState();
        PickAreaAction action(vps);

        const auto result = action.execute(
            {{"x0", 0.2}, {"y0", 0.3}, {"x1", 0.8}, {"y1", 0.9},
             {"coordType", "normalized"}, {"pickAction", "Add"}},
            nullptr);

        CHECK(result["ok"] == true);
        CHECK(result["async"] == true);

        const auto pending = vps.consumePickArea();
        REQUIRE(pending.has_value());
        CHECK(pending->x0 == doctest::Approx(0.2F));
        CHECK(pending->y1 == doctest::Approx(0.9F));
        CHECK(pending->coordType == OpenGeoLab::Scene::PickAreaCoordType::Normalized);
    }

    TEST_CASE("PickAreaAction stores pending pick with pixel coords") {
        SceneGraph graph;
        auto& vps = graph.viewportState();
        PickAreaAction action(vps);

        const auto result = action.execute(
            {{"x0", 100}, {"y0", 200}, {"x1", 400}, {"y1", 500},
             {"coordType", "pixel"}, {"pickAction", "Remove"}},
            nullptr);

        CHECK(result["ok"] == true);
        const auto pending = vps.consumePickArea();
        REQUIRE(pending.has_value());
        CHECK(pending->coordType == OpenGeoLab::Scene::PickAreaCoordType::Pixel);
        CHECK(pending->action == OpenGeoLab::Core::PickAction::Remove);
    }

    TEST_CASE("PickAreaAction defaults to normalized and Add") {
        SceneGraph graph;
        auto& vps = graph.viewportState();
        PickAreaAction action(vps);

        const auto result = action.execute(
            {{"x0", 0.0}, {"y0", 0.0}, {"x1", 1.0}, {"y1", 1.0}}, nullptr);

        CHECK(result["ok"] == true);
        const auto pending = vps.consumePickArea();
        REQUIRE(pending.has_value());
        CHECK(pending->coordType == OpenGeoLab::Scene::PickAreaCoordType::Normalized);
        CHECK(pending->action == OpenGeoLab::Core::PickAction::Add);
    }

} // TEST_SUITE
```

- [ ] **Step 2: Create fit_to_scene_action.hpp**

```cpp
/**
 * @file fit_to_scene_action.hpp
 * @brief FitToSceneAction — frame camera to show entire scene
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class SceneGraph;

/**
 * @brief Fit camera to the scene bounding box.
 *
 * Param: {} (no parameters required)
 */
class OPENGEOLAB_SCENE_EXPORT FitToSceneAction final : public Core::IAction {
public:
    explicit FitToSceneAction(SceneGraph& graph);
    ~FitToSceneAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr const char* ACTION_NAME = "fit_to_scene";

private:
    SceneGraph& m_graph;
};

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 3: Create fit_to_scene_action.cpp**

```cpp
/**
 * @file fit_to_scene_action.cpp
 * @brief FitToSceneAction implementation
 */

#include <opengeolab/scene/fit_to_scene_action.hpp>

#include <opengeolab/scene/scene_graph.hpp>

namespace OpenGeoLab::Scene {

FitToSceneAction::FitToSceneAction(SceneGraph& graph) : m_graph(graph) {}
FitToSceneAction::~FitToSceneAction() = default;

nlohmann::json FitToSceneAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Fit camera to the scene bounding box."},
        {"params", nlohmann::json::object()},
        {"returns",
         {{"ok", {{"type", "boolean"}, {"description", "Always true."}}}}}};
}

nlohmann::json FitToSceneAction::execute(const nlohmann::json& /*param*/,
                                         const Core::ProgressCallback& progress) {
    const auto bounds = m_graph.sceneBounds();
    m_graph.viewportState().fitToBounds(bounds);

    if(progress) {
        progress(1.0, "Done");
    }
    return {{"ok", true}, {"action", ACTION_NAME}};
}

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 4: Create set_view_preset_action.hpp**

```cpp
/**
 * @file set_view_preset_action.hpp
 * @brief SetViewPresetAction — apply a standard camera view preset
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class ViewportState;

/**
 * @brief Apply a named camera view preset.
 *
 * Param: {"preset": "Front"|"Back"|"Top"|"Bottom"|"Left"|"Right"|"Isometric"}
 */
class OPENGEOLAB_SCENE_EXPORT SetViewPresetAction final : public Core::IAction {
public:
    explicit SetViewPresetAction(ViewportState& state);
    ~SetViewPresetAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr const char* ACTION_NAME = "set_view_preset";

private:
    ViewportState& m_state;
};

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 5: Create set_view_preset_action.cpp**

```cpp
/**
 * @file set_view_preset_action.cpp
 * @brief SetViewPresetAction implementation
 */

#include <opengeolab/scene/set_view_preset_action.hpp>

#include <opengeolab/scene/viewport_state.hpp>

#include <optional>
#include <string>

namespace OpenGeoLab::Scene {

namespace {

std::optional<ViewPreset> parseViewPreset(const std::string& name) {
    if(name == "Front") return ViewPreset::Front;
    if(name == "Back") return ViewPreset::Back;
    if(name == "Top") return ViewPreset::Top;
    if(name == "Bottom") return ViewPreset::Bottom;
    if(name == "Left") return ViewPreset::Left;
    if(name == "Right") return ViewPreset::Right;
    if(name == "Isometric") return ViewPreset::Isometric;
    return std::nullopt;
}

} // namespace

SetViewPresetAction::SetViewPresetAction(ViewportState& state) : m_state(state) {}
SetViewPresetAction::~SetViewPresetAction() = default;

nlohmann::json SetViewPresetAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Apply a standard camera view preset."},
        {"params",
         {{"preset",
           {{"type", "string"},
            {"required", true},
            {"description",
             "One of: Front, Back, Top, Bottom, Left, Right, Isometric"}}}}},
        {"returns",
         {{"ok", {{"type", "boolean"}, {"description", "true on success."}}}}}};
}

nlohmann::json SetViewPresetAction::execute(const nlohmann::json& param,
                                            const Core::ProgressCallback& progress) {
    const auto preset_name = param.value("preset", std::string{});
    const auto preset = parseViewPreset(preset_name);
    if(!preset.has_value()) {
        return {{"ok", false},
                {"action", ACTION_NAME},
                {"error", "Unknown preset: " + preset_name}};
    }

    m_state.setViewPreset(*preset);

    if(progress) {
        progress(1.0, "Done");
    }
    return {{"ok", true}, {"action", ACTION_NAME}};
}

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 6: Create set_camera_action.hpp**

```cpp
/**
 * @file set_camera_action.hpp
 * @brief SetCameraAction — directly set camera position/target/up
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class ViewportState;

/**
 * @brief Set camera position, target, and up vectors directly.
 *
 * Param: {"position": [x,y,z], "target": [x,y,z], "up": [x,y,z]}
 */
class OPENGEOLAB_SCENE_EXPORT SetCameraAction final : public Core::IAction {
public:
    explicit SetCameraAction(ViewportState& state);
    ~SetCameraAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr const char* ACTION_NAME = "set_camera";

private:
    ViewportState& m_state;
};

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 7: Create set_camera_action.cpp**

```cpp
/**
 * @file set_camera_action.cpp
 * @brief SetCameraAction implementation
 */

#include <opengeolab/scene/set_camera_action.hpp>

#include <opengeolab/scene/viewport_state.hpp>

namespace OpenGeoLab::Scene {

SetCameraAction::SetCameraAction(ViewportState& state) : m_state(state) {}
SetCameraAction::~SetCameraAction() = default;

nlohmann::json SetCameraAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Set camera position, target, and up vectors."},
        {"params",
         {{"position",
           {{"type", "array"}, {"required", true}, {"description", "[x, y, z] eye position"}}},
          {"target",
           {{"type", "array"}, {"required", true}, {"description", "[x, y, z] look-at target"}}},
          {"up",
           {{"type", "array"}, {"required", true}, {"description", "[x, y, z] up direction"}}}}},
        {"returns",
         {{"ok", {{"type", "boolean"}, {"description", "true on success."}}}}}};
}

nlohmann::json SetCameraAction::execute(const nlohmann::json& param,
                                        const Core::ProgressCallback& progress) {
    if(!param.contains("position") || !param["position"].is_array() ||
       param["position"].size() != 3) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Missing or invalid 'position'"}};
    }
    if(!param.contains("target") || !param["target"].is_array() || param["target"].size() != 3) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Missing or invalid 'target'"}};
    }
    if(!param.contains("up") || !param["up"].is_array() || param["up"].size() != 3) {
        return {{"ok", false}, {"action", ACTION_NAME}, {"error", "Missing or invalid 'up'"}};
    }

    CameraState state;
    state.position = {param["position"][0].get<float>(), param["position"][1].get<float>(),
                      param["position"][2].get<float>()};
    state.target = {param["target"][0].get<float>(), param["target"][1].get<float>(),
                    param["target"][2].get<float>()};
    state.up = {param["up"][0].get<float>(), param["up"][1].get<float>(),
                param["up"][2].get<float>()};
    state.updateClipping();

    m_state.setCamera(state);

    if(progress) {
        progress(1.0, "Done");
    }
    return {{"ok", true}, {"action", ACTION_NAME}};
}

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 8: Create pick_area_action.hpp**

```cpp
/**
 * @file pick_area_action.hpp
 * @brief PickAreaAction — async box-select pick from command/Python
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/scene/scene_export.hpp>

namespace OpenGeoLab::Scene {

class ViewportState;

/**
 * @brief Queue a box-select pick area request (async).
 *
 * Results appear in SelectionState after the next render frame.
 * Query with "scene.query_selection".
 *
 * Param: {"x0", "y0", "x1", "y1", "coordType": "normalized"|"pixel",
 *         "pickAction": "Add"|"Remove"}
 */
class OPENGEOLAB_SCENE_EXPORT PickAreaAction final : public Core::IAction {
public:
    explicit PickAreaAction(ViewportState& state);
    ~PickAreaAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr const char* ACTION_NAME = "pick_area";

private:
    ViewportState& m_state;
};

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 9: Create pick_area_action.cpp**

```cpp
/**
 * @file pick_area_action.cpp
 * @brief PickAreaAction implementation
 */

#include <opengeolab/scene/pick_area_action.hpp>

#include <opengeolab/scene/viewport_state.hpp>

#include <string>

namespace OpenGeoLab::Scene {

PickAreaAction::PickAreaAction(ViewportState& state) : m_state(state) {}
PickAreaAction::~PickAreaAction() = default;

nlohmann::json PickAreaAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description",
         "Queue an async box-select pick area. Results appear in SelectionState after "
         "the next render frame. Query with scene.query_selection."},
        {"params",
         {{"x0", {{"type", "number"}, {"required", true}, {"description", "Left/start X"}}},
          {"y0", {{"type", "number"}, {"required", true}, {"description", "Top/start Y"}}},
          {"x1", {{"type", "number"}, {"required", true}, {"description", "Right/end X"}}},
          {"y1", {{"type", "number"}, {"required", true}, {"description", "Bottom/end Y"}}},
          {"coordType",
           {{"type", "string"},
            {"required", false},
            {"description", "'normalized' (0-1, default) or 'pixel' (item space)"}}},
          {"pickAction",
           {{"type", "string"},
            {"required", false},
            {"description", "'Add' (default) or 'Remove'"}}}}},
        {"returns",
         {{"ok", {{"type", "boolean"}}},
          {"async", {{"type", "boolean"}, {"description", "Always true — results are async."}}}}}};
}

nlohmann::json PickAreaAction::execute(const nlohmann::json& param,
                                       const Core::ProgressCallback& progress) {
    PendingPickArea request;
    request.x0 = param.value("x0", 0.0F);
    request.y0 = param.value("y0", 0.0F);
    request.x1 = param.value("x1", 0.0F);
    request.y1 = param.value("y1", 0.0F);

    const auto coord_type = param.value("coordType", std::string{"normalized"});
    if(coord_type == "pixel") {
        request.coordType = PickAreaCoordType::Pixel;
    } else {
        request.coordType = PickAreaCoordType::Normalized;
    }

    const auto pick_action = param.value("pickAction", std::string{"Add"});
    if(pick_action == "Remove") {
        request.action = Core::PickAction::Remove;
    } else {
        request.action = Core::PickAction::Add;
    }

    m_state.requestPickArea(request);

    if(progress) {
        progress(1.0, "Done");
    }
    return {{"ok", true}, {"action", ACTION_NAME}, {"async", true}};
}

} // namespace OpenGeoLab::Scene
```

- [ ] **Step 10: Add all new files to scene CMakeLists.txt**

In `scene_public_headers`, add (after `viewport_state.hpp`):
```
    include/opengeolab/scene/fit_to_scene_action.hpp
    include/opengeolab/scene/pick_area_action.hpp
    include/opengeolab/scene/set_camera_action.hpp
    include/opengeolab/scene/set_view_preset_action.hpp
```

In `scene_sources`, add (after `viewport_state.cpp`):
```
    src/fit_to_scene_action.cpp
    src/pick_area_action.cpp
    src/set_camera_action.cpp
    src/set_view_preset_action.cpp
```

Add test block (after `opengeolab_viewport_state_test`):
```cmake
    opengeolab_add_doctest_test(
        opengeolab_viewport_actions_test
        SOURCES test/viewport_actions_test.cpp
        LINKS OpenGeoLab::Scene)
```

- [ ] **Step 11: Build and run tests**

Run: `cmake --build build --config RelWithDebInfo --target opengeolab_scene --parallel 8`
Expected: Build succeeds.

Run: `ctest --test-dir build -C RelWithDebInfo -R viewport_actions --output-on-failure`
Expected: All tests PASS.

- [ ] **Step 12: Commit**

```
git add src/libs/scene/include/opengeolab/scene/fit_to_scene_action.hpp \
        src/libs/scene/include/opengeolab/scene/set_view_preset_action.hpp \
        src/libs/scene/include/opengeolab/scene/set_camera_action.hpp \
        src/libs/scene/include/opengeolab/scene/pick_area_action.hpp \
        src/libs/scene/src/fit_to_scene_action.cpp \
        src/libs/scene/src/set_view_preset_action.cpp \
        src/libs/scene/src/set_camera_action.cpp \
        src/libs/scene/src/pick_area_action.cpp \
        src/libs/scene/test/viewport_actions_test.cpp \
        src/libs/scene/CMakeLists.txt
git commit -m "feat(scene): add camera and pick area actions"
```

---

### Task 6: Register actions in SceneModule and connect ViewportState signals

**Files:**
- Modify: `src/libs/scene/src/scene_module.cpp`

- [ ] **Step 1: Update scene_module.cpp**

Add includes (after existing action includes):
```cpp
#include <opengeolab/scene/fit_to_scene_action.hpp>
#include <opengeolab/scene/pick_area_action.hpp>
#include <opengeolab/scene/set_camera_action.hpp>
#include <opengeolab/scene/set_view_preset_action.hpp>
```

Add action registrations in the constructor (after `registerAction<SetHoverAction>`):
```cpp
    registerAction<FitToSceneAction>(std::ref(m_sceneGraph));
    registerAction<SetViewPresetAction>(std::ref(m_sceneGraph.viewportState()));
    registerAction<SetCameraAction>(std::ref(m_sceneGraph.viewportState()));
    registerAction<PickAreaAction>(std::ref(m_sceneGraph.viewportState()));
```

Add ViewportState signal connections (after the existing SelectionState connections):
```cpp
    auto& vps = m_sceneGraph.viewportState();
    m_graphConnections.push_back(vps.cameraChanged.connect(
        [this]() { dataChanged.emit(Core::ModuleDataEvent::ItemModified); }));
    m_graphConnections.push_back(vps.pickAreaRequested.connect(
        [this]() { dataChanged.emit(Core::ModuleDataEvent::ItemModified); }));
```

- [ ] **Step 2: Build and run scene module test**

Run: `cmake --build build --config RelWithDebInfo --target opengeolab_scene --parallel 8`
Run: `ctest --test-dir build -C RelWithDebInfo -R scene_module --output-on-failure`
Expected: PASS.

- [ ] **Step 3: Run all scene tests**

Run: `ctest --test-dir build -C RelWithDebInfo -R scene --output-on-failure`
Expected: All PASS.

- [ ] **Step 4: Commit**

```
git add src/libs/scene/src/scene_module.cpp
git commit -m "feat(scene): register viewport actions and connect signals"
```

---

### Task 7: Adapt app layer — switch to scene CameraState

**Files:**
- Modify: `src/app/include/opengeolab/app/trackball_controller.hpp`
- Modify: `src/app/src/trackball_controller.cpp`
- Modify: `src/app/include/opengeolab/app/gl_viewport.hpp`
- Modify: `src/app/src/gl_viewport.cpp`
- Modify: `src/app/src/gl_viewport_renderer.cpp`
- Modify: `src/app/CMakeLists.txt`
- Delete: `src/app/include/opengeolab/app/camera_state.hpp`
- Delete: `src/app/src/camera_state.cpp`
- Modify: `src/app/test/camera_state_test.cpp` (redirect to scene)
- Modify: `src/app/test/trackball_controller_test.cpp` (update for removed methods)

- [ ] **Step 1: Update trackball_controller.hpp**

Replace the entire file content. Key changes:
- Include `<opengeolab/scene/camera_state.hpp>` and `<opengeolab/scene/view_preset.hpp>` instead of `<opengeolab/app/camera_state.hpp>`
- Add `using OpenGeoLab::Scene::CameraState;` inside App namespace
- Remove `ViewPreset` enum (now in `Scene::ViewPreset`)
- Remove `fitToScene()` and `setViewPreset()` methods
- Remove `#include <opengeolab/scene/bounding_box3d.hpp>` (no longer needed directly)

```cpp
/**
 * @file trackball_controller.hpp
 * @brief Quaternion-based trackball controller for orbit/pan/zoom
 */

#pragma once

#include <opengeolab/scene/camera_state.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace OpenGeoLab::App {

using OpenGeoLab::Scene::CameraState;

/**
 * @brief Trackball controller for 3D camera manipulation
 *
 * Supports orbit (virtual sphere), exponential zoom, and view-plane pan.
 * All manipulation is applied to a CameraState reference.
 */
class TrackballController {
public:
    enum class Mode { None, Orbit, Pan, Zoom };

    /** @brief Set viewport size for mouse normalization */
    void setViewportSize(float width, float height);

    /** @brief Check if a drag operation is active */
    [[nodiscard]] bool isActive() const { return m_mode != Mode::None; }

    /** @brief Current interaction mode */
    [[nodiscard]] Mode mode() const { return m_mode; }

    /** @brief Start a drag operation */
    void begin(float x, float y, Mode mode, const CameraState& state);

    /** @brief Update during drag — modifies camera state */
    void update(float x, float y, CameraState& state);

    /** @brief End drag operation */
    void end();

    /** @brief Wheel zoom — modifies camera state */
    void wheelZoom(float steps, CameraState& state);

private:
    /** @brief Project screen point to virtual sphere */
    [[nodiscard]] glm::vec3 projectToSphere(float x, float y) const;

    void applyOrbit(float x, float y, CameraState& state);
    void applyPan(float x, float y, CameraState& state);
    void applyZoom(float x, float y, CameraState& state);

    Mode m_mode{Mode::None};
    float m_viewportWidth{1.0F};
    float m_viewportHeight{1.0F};

    float m_lastX{0.0F};
    float m_lastY{0.0F};
    float m_startDistance{1.0F};

    static constexpr float ORBIT_SCALE = 2.2F;
    static constexpr float PAN_SCALE = 0.0015F;
    static constexpr float ZOOM_BASE = 0.90F;
    static constexpr float ZOOM_SPEED = 1.5F;
    static constexpr float ZOOM_PIXELS_PER_STEP = 60.0F;
    static constexpr float MIN_DISTANCE = 0.1F;
};

} // namespace OpenGeoLab::App
```

- [ ] **Step 2: Update trackball_controller.cpp**

Replace the entire file. Key changes:
- Include `<opengeolab/app/trackball_controller.hpp>` (unchanged)
- Remove `fitToScene()` and `setViewPreset()` implementations (now in ViewportState)

```cpp
/**
 * @file trackball_controller.cpp
 * @brief TrackballController implementation — orbit, pan, zoom
 */

#include <opengeolab/app/trackball_controller.hpp>

#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>

namespace OpenGeoLab::App {

void TrackballController::setViewportSize(float width, float height) {
    m_viewportWidth = std::max(width, 1.0F);
    m_viewportHeight = std::max(height, 1.0F);
}

void TrackballController::begin(float x, float y, Mode mode, const CameraState& state) {
    m_mode = mode;
    m_lastX = x;
    m_lastY = y;
    m_startDistance = state.distance();
}

void TrackballController::update(float x, float y, CameraState& state) {
    switch(m_mode) {
    case Mode::Orbit:
        applyOrbit(x, y, state);
        break;
    case Mode::Pan:
        applyPan(x, y, state);
        break;
    case Mode::Zoom:
        applyZoom(x, y, state);
        break;
    case Mode::None:
        break;
    }
    m_lastX = x;
    m_lastY = y;
}

void TrackballController::end() { m_mode = Mode::None; }

glm::vec3 TrackballController::projectToSphere(float x, float y) const {
    const float normalized_x = (2.0F * x / m_viewportWidth - 1.0F);
    const float normalized_y = (1.0F - 2.0F * y / m_viewportHeight);

    const float r2 = normalized_x * normalized_x + normalized_y * normalized_y;

    float z = 0.0F;
    if(r2 <= 0.5F) {
        z = std::sqrt(1.0F - r2);
    } else {
        z = 0.5F / std::sqrt(r2);
    }

    const float inv_norm = 1.0F / std::sqrt(r2 + z * z);
    return {normalized_x * inv_norm, normalized_y * inv_norm, z * inv_norm};
}

void TrackballController::applyOrbit(float x, float y, CameraState& state) {
    const glm::vec3 from = projectToSphere(m_lastX, m_lastY);
    const glm::vec3 to = projectToSphere(x, y);

    glm::vec3 axis = glm::cross(from, to);
    const float axis_length = glm::length(axis);
    if(axis_length < 1.0e-6F) {
        return;
    }

    axis /= axis_length;
    const float angle = -ORBIT_SCALE * std::asin(std::clamp(axis_length, -1.0F, 1.0F));

    const glm::vec3 view_direction = glm::normalize(state.position - state.target);
    const glm::vec3 right = glm::normalize(glm::cross(state.up, view_direction));
    const glm::vec3 true_up = glm::cross(view_direction, right);
    const glm::mat3 view_to_world(right, true_up, view_direction);
    const glm::vec3 world_axis = view_to_world * axis;
    const glm::quat world_rotation = glm::angleAxis(angle, world_axis);

    const float camera_distance = state.distance();
    glm::vec3 offset = state.position - state.target;
    offset = world_rotation * offset;
    state.position = state.target + glm::normalize(offset) * camera_distance;
    state.up = world_rotation * state.up;
    state.updateClipping();
}

void TrackballController::applyPan(float x, float y, CameraState& state) {
    const float delta_x = x - m_lastX;
    const float delta_y = y - m_lastY;

    const glm::vec3 view_direction = glm::normalize(state.target - state.position);
    const glm::vec3 right = glm::normalize(glm::cross(view_direction, state.up));
    const glm::vec3 up = glm::cross(right, view_direction);

    const float scale = state.distance() * PAN_SCALE;
    const glm::vec3 pan = (-delta_x * right + delta_y * up) * scale;
    state.position += pan;
    state.target += pan;
}

void TrackballController::applyZoom(float /*x*/, float y, CameraState& state) {
    const float delta_y = y - m_lastY;
    const float steps = -delta_y / ZOOM_PIXELS_PER_STEP;
    const float factor = std::pow(ZOOM_BASE, steps * ZOOM_SPEED);

    float distance = state.distance() * factor;
    distance = std::max(distance, MIN_DISTANCE);

    const glm::vec3 direction = glm::normalize(state.position - state.target);
    state.position = state.target + direction * distance;
    state.updateClipping();
}

void TrackballController::wheelZoom(float steps, CameraState& state) {
    const float factor = std::pow(ZOOM_BASE, steps * ZOOM_SPEED);
    float distance = state.distance() * factor;
    distance = std::max(distance, MIN_DISTANCE);

    const glm::vec3 direction = glm::normalize(state.position - state.target);
    state.position = state.target + direction * distance;
    state.updateClipping();
}

} // namespace OpenGeoLab::App
```

- [ ] **Step 3: Update gl_viewport.hpp**

Key changes:
- Remove `#include <opengeolab/app/camera_state.hpp>` (comes through trackball_controller.hpp)
- Remove `CameraState m_camera;` member
- Remove `cameraState()` accessors
- Keep `fitToScene()` and `setViewPreset()` Q_INVOKABLE (they now delegate to ViewportState)

Replace the include at line 8:
```cpp
// Remove: #include <opengeolab/app/camera_state.hpp>
```
(The trackball_controller.hpp include at line 9 already brings in CameraState.)

Remove the cameraState accessors (lines 67-70):
```cpp
    // REMOVED: cameraState() — camera is now in SceneGraph::viewportState()
```

Remove the `CameraState m_camera;` member (line 184).

- [ ] **Step 4: Update gl_viewport.cpp**

Key changes:
- Constructor: remove `m_camera.reset();` (line 36)
- `fitToScene()`: delegate to `m_sceneGraph->viewportState().fitToBounds()`
- `setViewPreset()`: delegate to `m_sceneGraph->viewportState().setViewPreset()`
- Mouse events: read-modify-writeback via ViewportState
- All references to `m_camera` replaced with ViewportState access

Replace `fitToScene()` (lines 122-129):
```cpp
void GLViewport::fitToScene() {
    if(m_sceneGraph == nullptr) {
        return;
    }

    m_sceneGraph->viewportState().fitToBounds(m_sceneGraph->sceneBounds());
    update();
}
```

Replace `setViewPreset()` (lines 131-139):
```cpp
void GLViewport::setViewPreset(int preset) {
    if(m_sceneGraph == nullptr) {
        return;
    }
    if(preset < static_cast<int>(Scene::ViewPreset::Front) ||
       preset > static_cast<int>(Scene::ViewPreset::Isometric)) {
        return;
    }

    m_sceneGraph->viewportState().setViewPreset(static_cast<Scene::ViewPreset>(preset));
    update();
}
```

Replace mouse move orbit/pan handling. In `mouseMoveEvent()`, where `m_trackball.begin(...)` and `m_trackball.update(...)` are called (lines 170-177), change to:
```cpp
            auto state = m_sceneGraph->viewportState().camera();
            m_trackball.begin(static_cast<float>(m_pressPos.x()),
                              static_cast<float>(m_pressPos.y()), mode, state);
            m_trackball.update(static_cast<float>(event->position().x()),
                               static_cast<float>(event->position().y()), state);
            m_sceneGraph->viewportState().setCamera(state);
```

Where `m_trackball.update(...)` is called during active drag (lines 152-157), change to:
```cpp
    if(m_trackball.isActive()) {
        if(m_sceneGraph != nullptr) {
            auto state = m_sceneGraph->viewportState().camera();
            m_trackball.update(static_cast<float>(event->position().x()),
                               static_cast<float>(event->position().y()), state);
            m_sceneGraph->viewportState().setCamera(state);
        }
        update();
        event->accept();
        return;
    }
```

Where `m_trackball.wheelZoom(...)` is called (lines 234-237), change to:
```cpp
            if(m_sceneGraph != nullptr) {
                auto state = m_sceneGraph->viewportState().camera();
                m_trackball.wheelZoom(steps * 2.0F, state);
                m_sceneGraph->viewportState().setCamera(state);
            }
```

Remove `m_camera.reset();` from constructor (line 36). Add `#include <opengeolab/scene/viewport_state.hpp>` if needed (may come transitively).

- [ ] **Step 5: Update gl_viewport_renderer.cpp**

Key changes:
- Camera source: read from `scene->viewportState().camera()` instead of `viewport->cameraState()`
- Consume pick area from ViewportState

Replace camera read in `synchronize()` (lines 96-103):
```cpp
    const auto camera = (viewport->sceneGraph() != nullptr)
        ? viewport->sceneGraph()->viewportState().camera()
        : Scene::CameraState{};
    m_frameState.viewMatrix = camera.viewMatrix();
    m_frameState.projMatrix = camera.projMatrix(aspect);
    m_frameState.cameraPos = camera.eyePosition();
    m_frameState.viewportWidth = static_cast<int>(width_px);
    m_frameState.viewportHeight = static_cast<int>(height_px);
    m_frameState.devicePixelRatio = device_pixel_ratio;
    m_frameState.xRayMode = viewport->xRayMode();
```

Add pick area consumption in `synchronize()` (after line 108, where box select is consumed):
```cpp
    if(const auto* scene = viewport->sceneGraph(); scene != nullptr) {
        m_pendingPickArea = scene->viewportState().consumePickArea();
    }
```

Add `m_pendingPickArea` member to the class (in the header `gl_viewport_renderer.hpp`):
```cpp
    std::optional<Scene::PendingPickArea> m_pendingPickArea;
```

Add include in gl_viewport_renderer.hpp:
```cpp
#include <opengeolab/scene/viewport_state.hpp>
```

In `render()`, after processing `m_pendingBoxSelect` (line 191-194), add pick area dispatch:
```cpp
        if(m_pendingPickArea.has_value()) {
            dispatchPickAreaResults(*m_pendingPickArea);
            m_pendingPickArea.reset();
        }
```

Add the `dispatchPickAreaResults` method (after `dispatchBoxSelectResults`):
```cpp
void GLViewportRenderer::dispatchPickAreaResults(const Scene::PendingPickArea& area) const {
    if(m_viewport.isNull()) {
        return;
    }

    float fx0 = area.x0;
    float fy0 = area.y0;
    float fx1 = area.x1;
    float fy1 = area.y1;

    if(area.coordType == Scene::PickAreaCoordType::Normalized) {
        fx0 *= static_cast<float>(m_frameState.viewportWidth);
        fy0 *= static_cast<float>(m_frameState.viewportHeight);
        fx1 *= static_cast<float>(m_frameState.viewportWidth);
        fy1 *= static_cast<float>(m_frameState.viewportHeight);
    } else {
        const float dpr = m_frameState.devicePixelRatio;
        fx0 *= dpr;
        fy0 *= dpr;
        fx1 *= dpr;
        fy1 *= dpr;
    }

    auto results = m_pipeline.pickRect(
        static_cast<int>(fx0), static_cast<int>(fy0),
        static_cast<int>(fx1), static_cast<int>(fy1), pickMask());
    if(results.empty()) {
        return;
    }

    std::vector<Core::EntityRef> entities;
    entities.reserve(results.size());
    for(const auto& r : results) {
        if(r.valid) {
            if((Core::maskForEntityType(r.entityType) & pickMask()) == Core::PickMask::None) {
                continue;
            }
            entities.push_back({r.shapeId, r.entityType, r.localId});
        }
    }

    const QPointer<GLViewport> viewport = m_viewport;
    const Core::PickAction action = area.action;
    QMetaObject::invokeMethod(
        viewport.data(),
        [viewport, entities = std::move(entities), action]() {
            if(viewport.isNull() || viewport->sceneGraph() == nullptr) {
                return;
            }
            auto& sel = viewport->sceneGraph()->selectionState();
            for(const auto& entity : entities) {
                if(action == Core::PickAction::Add) {
                    sel.addSelection(entity);
                } else {
                    sel.removeSelection(entity);
                }
            }
        },
        Qt::QueuedConnection);
}
```

Also declare `dispatchPickAreaResults` in the header.

- [ ] **Step 6: Delete old app camera_state files**

```
git rm src/app/include/opengeolab/app/camera_state.hpp
git rm src/app/src/camera_state.cpp
```

- [ ] **Step 7: Update app CMakeLists.txt**

Remove `src/camera_state.cpp` from `qt_add_executable` sources (line 16).

Update `opengeolab_camera_state_test`: redirect to test the scene CameraState:
```cmake
    opengeolab_add_doctest_test(
        opengeolab_camera_state_test SOURCES test/camera_state_test.cpp
        LINKS OpenGeoLab::Scene)
    target_include_directories(opengeolab_camera_state_test
                               PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
```
(Remove `src/camera_state.cpp` from SOURCES — it's now in Scene library.)

Update `opengeolab_trackball_controller_test`: remove `src/camera_state.cpp` from SOURCES:
```cmake
    opengeolab_add_doctest_test(
        opengeolab_trackball_controller_test
        SOURCES
        test/trackball_controller_test.cpp
        src/trackball_controller.cpp
        LINKS
        OpenGeoLab::Scene)
```

Update `opengeolab_gl_viewport_test`: remove `src/camera_state.cpp` from SOURCES:
```cmake
    opengeolab_add_doctest_test(
        opengeolab_gl_viewport_test
        SOURCES
        test/gl_viewport_test.cpp
        include/opengeolab/app/gl_viewport.hpp
        include/opengeolab/app/gl_viewport_renderer.hpp
        src/gl_viewport.cpp
        src/gl_viewport_renderer.cpp
        src/trackball_controller.cpp
        LINKS
        ...same as before...)
```

- [ ] **Step 8: Update app camera_state_test.cpp to use Scene namespace**

Replace includes and namespace to use Scene::CameraState. Since the test file now links Scene, just change the include and namespace:

```cpp
#include <opengeolab/scene/camera_state.hpp>
```
And namespace to `OpenGeoLab::Scene::Tests`. Update any `App::` qualifiers.

- [ ] **Step 9: Update trackball_controller_test.cpp**

Change include from `<opengeolab/app/camera_state.hpp>` to just use the trackball header (which includes scene camera_state).

Remove the last test case `"TrackballController fit and view presets delegate to CameraState conventions"` (lines 96-116) since `fitToScene()` and `setViewPreset()` no longer exist on TrackballController.

Also add `#include <opengeolab/scene/bounding_box3d.hpp>` if still needed by remaining tests (it shouldn't be needed after removing the fit/preset test).

- [ ] **Step 10: Build full project**

Run: `cmake --build build --config RelWithDebInfo --parallel 8`
Expected: Clean build.

- [ ] **Step 11: Run all tests**

Run: `ctest --test-dir build -C RelWithDebInfo --output-on-failure`
Expected: All tests PASS.

- [ ] **Step 12: Commit**

```
git add -A
git commit -m "refactor(app): switch to scene CameraState, remove local camera copy

GLViewport reads/writes camera through SceneGraph::viewportState().
TrackballController uses read-modify-writeback pattern.
Renderer reads camera and pick area from ViewportState.
fitToScene/setViewPreset removed from TrackballController (now in ViewportState)."
```

---

### Task 8: Update Python demo plugin

**Files:**
- Modify: `plugins/selection_demo_plugin/__init__.py`

- [ ] **Step 1: Update plugin to use new viewport actions**

Update the plugin to include camera control and pick area buttons.
Add buttons for: Fit to Scene, Set View Preset, Pick Area, Set Camera.

After creating a box, auto-call fit_to_scene.
Add a combo box for view presets.
Add pick area button with configurable normalized coordinates.

The key additions are:
- After `create_box` succeeds: call `fit_to_scene` and optionally `set_view_preset`
- New "Fit to Scene" button: sends `{"module":"scene","action":"fit_to_scene"}`
- New view preset combo: sends `{"module":"scene","action":"set_view_preset","param":{"preset":"..."}}`
- New "Pick Area" button: sends `{"module":"scene","action":"pick_area","param":{...}}` with normalized coordinates from spin boxes
- After pick_area, use a QTimer to query selection results

- [ ] **Step 2: Copy plugin to build dir**

```
cmake -E copy_directory plugins build/bin/plugins
```

- [ ] **Step 3: Commit**

```
git add plugins/selection_demo_plugin/__init__.py
git commit -m "feat(python): update selection demo with camera and pick area controls"
```

---

### Task 9: Final verification and cleanup

- [ ] **Step 1: Full clean build**

Run: `cmake --build build --config RelWithDebInfo --parallel 8`
Expected: Clean build, zero warnings related to our changes.

- [ ] **Step 2: Run all tests**

Run: `ctest --test-dir build -C RelWithDebInfo --output-on-failure`
Expected: All tests PASS (should be ~31+ tests now with new ones).

- [ ] **Step 3: Verify git status is clean**

```
git --no-pager status
git --no-pager log --oneline -5
```

- [ ] **Step 4: Squash or organize commits if desired**

The implementation should have produced these commits:
1. `feat(scene): add ViewPreset enum for camera presets`
2. `feat(scene): migrate CameraState from app to scene layer`
3. `feat(scene): add ViewportState with mutex-protected camera and pick area`
4. `feat(scene): add camera and pick area actions`
5. `feat(scene): register viewport actions and connect signals`
6. `refactor(app): switch to scene CameraState, remove local camera copy`
7. `feat(python): update selection demo with camera and pick area controls`
