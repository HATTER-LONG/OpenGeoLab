#pragma once

/**
 * @file create_action.hpp
 * @brief Geometry creation action for primitive shapes
 *
 * CreateAction provides functionality to create various primitive shapes
 * (box, sphere, cylinder, cone) and add them to the current document.
 */

#include "geometry/geometry_action.hpp"

namespace OpenGeoLab::Geometry {

/**
 * @brief Action for creating primitive geometry shapes
 *
 * Supported shape types (via "type" parameter):
 * - "box": Create a box with "dx", "dy", "dz" dimensions
 * - "sphere": Create a sphere with "radius"
 * - "cylinder": Create a cylinder with "radius" and "height"
 * - "cone": Create a cone with "radius1", "radius2", and "height"
 *
 * Optional parameters:
 * - "name": Custom name for the created part
 */
class CreateAction : public GeometryActionBase {
public:
    CreateAction() = default;
    ~CreateAction() override = default;

    /**
     * @brief Get the action identifier
     * @return Action name string "create"
     */
    [[nodiscard]] static std::string actionName() { return "create"; }

    /**
     * @brief Execute the creation action
     * @param params JSON parameters containing shape type and dimensions
     * @param progress_callback Progress reporting callback
     * @return true if creation succeeded, false otherwise
     *
     * Expected params structure:
     * @code
     * {
     *   "type": "box",        // or "sphere", "cylinder", "cone"
     *   "dx": 10.0,           // for box
     *   "dy": 10.0,           // for box
     *   "dz": 10.0,           // for box
     *   "radius": 5.0,        // for sphere/cylinder/cone
     *   "radius1": 5.0,       // for cone (bottom)
     *   "radius2": 2.0,       // for cone (top)
     *   "height": 10.0,       // for cylinder/cone
     *   "name": "MyPart"      // optional
     * }
     * @endcode
     */
    [[nodiscard]] bool execute(const nlohmann::json& params,
                               Util::ProgressCallback progress_callback) override;

private:
    /**
     * @brief Create a box shape
     * @param params Parameters with dx, dy, dz
     * @param name Part name
     * @param progress_callback Progress callback
     * @return true on success
     */
    [[nodiscard]] bool createBox(const nlohmann::json& params,
                                 const std::string& name,
                                 Util::ProgressCallback& progress_callback);

    /**
     * @brief Create a sphere shape
     * @param params Parameters with radius
     * @param name Part name
     * @param progress_callback Progress callback
     * @return true on success
     */
    [[nodiscard]] bool createSphere(const nlohmann::json& params,
                                    const std::string& name,
                                    Util::ProgressCallback& progress_callback);

    /**
     * @brief Create a cylinder shape
     * @param params Parameters with radius and height
     * @param name Part name
     * @param progress_callback Progress callback
     * @return true on success
     */
    [[nodiscard]] bool createCylinder(const nlohmann::json& params,
                                      const std::string& name,
                                      Util::ProgressCallback& progress_callback);

    /**
     * @brief Create a cone shape
     * @param params Parameters with radius1, radius2, and height
     * @param name Part name
     * @param progress_callback Progress callback
     * @return true on success
     */
    [[nodiscard]] bool createCone(const nlohmann::json& params,
                                  const std::string& name,
                                  Util::ProgressCallback& progress_callback);
};

/**
 * @brief Factory for creating CreateAction instances
 */
class CreateActionFactory : public GeometryActionFactory {
public:
    CreateActionFactory() = default;
    ~CreateActionFactory() = default;

    tObjectPtr create() override { return std::make_unique<CreateAction>(); }
};

} // namespace OpenGeoLab::Geometry