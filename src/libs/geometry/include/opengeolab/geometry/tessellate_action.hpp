/**
 * @file tessellate_action.hpp
 * @brief TessellateAction — explicitly tessellate a shape in ShapeStore
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

namespace OpenGeoLab::Geometry {

class ShapeStore;

/**
 * @brief Explicitly tessellates a shape and caches the visual data in ShapeStore.
 */
class OPENGEOLAB_GEOMETRY_EXPORT TessellateAction final : public Core::IAction {
public:
    /**
     * @param store ShapeStore that manages shape lifecycle
     */
    explicit TessellateAction(ShapeStore& store);
    ~TessellateAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"tessellate"};

private:
    ShapeStore& m_store;
};

} // namespace OpenGeoLab::Geometry
