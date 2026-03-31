/**
 * @file pick_component.hpp
 * @brief IPickComponent — abstract interface for pickable scene nodes
 */

#pragma once

#include <opengeolab/scene/render_mesh_data.hpp>
#include <opengeolab/scene/scene_export.hpp>

#include <glm/glm.hpp>

#include <cstdint>
#include <optional>
#include <span>

namespace OpenGeoLab::Scene {

/** @brief Strategy for picking this node. */
enum class PickStrategy : uint8_t {
    Gpu,  /**< GPU color picking only */
    Ray,  /**< CPU ray intersection only */
    Both, /**< Both methods available */
};

/** @brief Ray in 3D space. */
struct Ray3D {
    glm::vec3 origin;    /**< Ray origin */
    glm::vec3 direction; /**< Ray direction (not necessarily normalized) */
};

/** @brief Result of a pick hit. */
struct PickHit {
    float distance{};  /**< Distance along the ray */
    uint64_t pickId{}; /**< Encoded pick identifier */
};

/**
 * @brief Interface for scene nodes that support picking.
 *
 * GPU picking uses pickEntries() to match framebuffer output.
 * Ray picking uses rayPick() for CPU fallback.
 */
class OPENGEOLAB_SCENE_EXPORT IPickComponent {
public:
    virtual ~IPickComponent() = default;

    /** @brief Picking method supported by this component. */
    [[nodiscard]] virtual PickStrategy strategy() const = 0;

    /** @brief Pick ID entries for GPU color picking. */
    [[nodiscard]] virtual std::span<const PickIdEntry> pickEntries() const = 0;

    /**
     * @brief CPU ray intersection (optional).
     *
     * Default returns std::nullopt. Override for Ray or Both strategies.
     */
    [[nodiscard]] virtual std::optional<PickHit> rayPick(const Ray3D& ray) const;
};

inline std::optional<PickHit> IPickComponent::rayPick(const Ray3D& /*ray*/) const {
    return std::nullopt;
}

} // namespace OpenGeoLab::Scene
