/**
 * @file newmodel_action.hpp
 * @brief Action for creating a new empty model
 *
 * NewModelAction clears the current document and creates a fresh empty model.
 * This is equivalent to the "File -> New" operation in a CAD application.
 */

#pragma once

#include "geometry/geometry_action.hpp"

namespace OpenGeoLab::Geometry {

/**
 * @brief Action for creating a new empty model
 *
 * This action clears all existing geometry from the current document,
 * effectively creating a fresh workspace. No parameters are required.
 */
class NewModelAction : public GeometryActionBase {
public:
    NewModelAction() = default;
    ~NewModelAction() override = default;

    /**
     * @brief Get the action identifier
     * @return Action name string "new_model"
     */
    [[nodiscard]] static std::string actionName() { return "new_model"; }

    /**
     * @brief Execute the new model action
     * @param params JSON parameters (not used)
     * @param progress_callback Progress reporting callback
     * @return JSON with "success" boolean and status info
     */
    [[nodiscard]] nlohmann::json execute(const nlohmann::json& params,
                                         Util::ProgressCallback progress_callback) override;
};

/**
 * @brief Factory for creating NewModelAction instances
 */
class NewModelActionFactory : public GeometryActionFactory {
public:
    NewModelActionFactory() = default;
    ~NewModelActionFactory() = default;

    tObjectPtr create() override { return std::make_unique<NewModelAction>(); }
};

} // namespace OpenGeoLab::Geometry
