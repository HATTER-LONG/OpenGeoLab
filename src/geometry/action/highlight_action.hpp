/**
 * @file highlight_action.hpp
 * @brief Action for highlighting geometry entities
 *
 * HighlightAction manages the visual highlighting of geometry entities
 * in the viewport. Supports preview (hover) and selection states,
 * with batch operations for efficient box selection.
 */

#pragma once

#include "geometry/geometry_action.hpp"

namespace OpenGeoLab::Geometry {

/**
 * @brief Highlight state enumeration
 */
enum class HighlightState : uint8_t {
    None = 0,    ///< No highlight (normal state)
    Preview = 1, ///< Preview highlight (mouse hover)
    Selected = 2 ///< Selected highlight (clicked/confirmed)
};

/**
 * @brief Action for managing entity highlight states
 *
 * Provides operations to:
 * - Set highlight state for single or multiple entities
 * - Clear all highlights
 * - Query current highlight states
 */
class HighlightAction : public GeometryActionBase {
public:
    HighlightAction() = default;
    ~HighlightAction() override = default;

    /**
     * @brief Get the action identifier
     * @return Action name string "highlight"
     */
    [[nodiscard]] static std::string actionName() { return "highlight"; }

    /**
     * @brief Execute the highlight action
     * @param params JSON parameters:
     *        - operation: "set", "clear", "clear_all", "get"
     *        - entity_id: single EntityId (for single ops)
     *        - entity_ids: array of EntityIds (for batch ops)
     *        - state: "none", "preview", "selected"
     * @param progress_callback Progress reporting callback
     * @return JSON with operation result
     *
     * Example requests:
     *
     * Set single entity preview:
     * { "operation": "set", "entity_id": 123, "state": "preview" }
     *
     * Set multiple entities selected:
     * { "operation": "set", "entity_ids": [123, 124, 125], "state": "selected" }
     *
     * Clear specific entities:
     * { "operation": "clear", "entity_ids": [123, 124] }
     *
     * Clear all highlights:
     * { "operation": "clear_all" }
     *
     * Get current highlights:
     * { "operation": "get" }
     */
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& params,
                                         Util::ProgressCallback progress_callback) override;
};

/**
 * @brief Factory for creating HighlightAction instances
 */
class HighlightActionFactory : public GeometryActionFactory {
public:
    HighlightActionFactory() = default;
    ~HighlightActionFactory() = default;

    tObjectPtr create() override { return std::make_unique<HighlightAction>(); }
};

} // namespace OpenGeoLab::Geometry
