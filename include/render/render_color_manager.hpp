/**
 * @file render_color_manager.hpp
 * @brief Global render color management for geometry and mesh entities
 */

#pragma once

#include "render/render_data.hpp"
#include "render/render_types.hpp"

#include <cstdint>

namespace OpenGeoLab::Render {

enum class RenderColorRole : uint8_t {
    Face = 0,
    Edge = 1,
    Vertex = 2,
    MeshNode = 3,
    MeshLine = 4,
    MeshElement = 5,
};

enum class RenderColorState : uint8_t {
    Normal = 0,
    Hover = 1,
    Selected = 2,
};

class RenderColorManager {
public:
    [[nodiscard]] static RenderColor colorForPart(
        uint64_t part_id, RenderColorRole role, RenderColorState state = RenderColorState::Normal);

    [[nodiscard]] static RenderColor
    colorForEntity(RenderEntityType entity_type,
                   uint64_t entity_uid,
                   uint64_t part_id = 0,
                   RenderColorState state = RenderColorState::Normal);
};

} // namespace OpenGeoLab::Render
