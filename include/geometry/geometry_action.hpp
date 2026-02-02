/**
 * @file geometry_action.hpp
 * @brief Base classes for geometry action commands
 *
 * Provides the foundation for implementing geometry modification commands
 * that can be executed through the GeometryService. Actions follow the
 * Command pattern and support progress reporting.
 */

#pragma once

#include "util/progress_callback.hpp"

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/noncopyable.hpp>
#include <nlohmann/json.hpp>

namespace OpenGeoLab::Geometry {

/**
 * @brief Abstract base class for geometry action commands
 *
 * GeometryActionBase defines the interface for all geometry operations
 * that can be executed through the GeometryService. Derived classes
 * implement specific operations such as shape creation, modification,
 * or document management.
 */
class GeometryActionBase : public Kangaroo::Util::NonCopyMoveable {
public:
    GeometryActionBase() = default;
    virtual ~GeometryActionBase() = default;

    /**
     * @brief Execute the geometry action
     * @param params Action-specific JSON parameters
     * @param progress_callback Optional callback for progress reporting
     * @return JSON result with "success" boolean and action-specific data
     * @note Implementations should report progress through the callback.
     *       Return {"success": false, "error": "message"} on failure.
     */
    [[nodiscard]] virtual nlohmann::json execute(const nlohmann::json& params,
                                                 Util::ProgressCallback progress_callback) = 0;
};

/**
 * @brief Factory interface for creating geometry action instances
 *
 * Actions are registered with the component factory using unique identifiers.
 * GeometryService uses these factories to instantiate actions based on
 * the "action" parameter in incoming requests.
 */
class GeometryActionFactory
    : public Kangaroo::Util::FactoryTraits<GeometryActionFactory, GeometryActionBase> {
public:
    GeometryActionFactory() = default;
    ~GeometryActionFactory() = default;

    /**
     * @brief Create a new action instance
     * @return Unique pointer to the created action
     */
    virtual tObjectPtr create() = 0;
};

} // namespace OpenGeoLab::Geometry