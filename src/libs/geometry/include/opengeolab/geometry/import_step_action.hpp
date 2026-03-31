/**
 * @file import_step_action.hpp
 * @brief ImportStepAction — imports a STEP file into ShapeStore
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/geometry/geometry_export.hpp>

namespace OpenGeoLab::Geometry {

class ShapeStore;

/**
 * @brief Imports a STEP file and registers the resulting shape in ShapeStore.
 */
class OPENGEOLAB_GEOMETRY_EXPORT ImportStepAction final : public Core::IAction {
public:
    /**
     * @param store ShapeStore that manages shape lifecycle
     */
    explicit ImportStepAction(ShapeStore& store);
    ~ImportStepAction() override;

    [[nodiscard]] nlohmann::json describe() const override;
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"import_step"};

private:
    ShapeStore& m_store;
};

} // namespace OpenGeoLab::Geometry
