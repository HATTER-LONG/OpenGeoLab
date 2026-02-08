/**
 * @file mesh_action.hpp
 * @brief Base classes for mesh action commands
 */

#pragma once

#include "util/progress_callback.hpp"

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/noncopyable.hpp>
#include <nlohmann/json.hpp>

namespace OpenGeoLab::Mesh {

/**
 * @brief Abstract base class for mesh actions dispatched by MeshService
 */
class MeshActionBase : public Kangaroo::Util::NonCopyMoveable {
public:
    MeshActionBase() = default;
    virtual ~MeshActionBase() = default;

    /**
     * @brief Execute the mesh action
     * @param params Action-specific JSON parameters
     * @param progress_callback Optional callback for progress reporting
     * @return JSON result with "success" boolean and action-specific data
     */
    [[nodiscard]] virtual nlohmann::json execute(const nlohmann::json& params,
                                                 Util::ProgressCallback progress_callback) = 0;
};

/**
 * @brief Factory interface for creating mesh action instances
 */
class MeshActionFactory : public Kangaroo::Util::FactoryTraits<MeshActionFactory, MeshActionBase> {
public:
    MeshActionFactory() = default;
    ~MeshActionFactory() = default;

    virtual tObjectPtr create() = 0;
};

} // namespace OpenGeoLab::Mesh
