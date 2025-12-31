/**
 * @file service.hpp
 * @brief Service layer interfaces for backend request processing
 */
#pragma once

#include <kangaroo/util/component_factory.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <string>

namespace OpenGeoLab {
namespace App {

/**
 * @brief Callback interface for reporting operation progress to the UI layer
 *
 * Allows services to report progress, messages, and errors without direct
 * coupling to Qt or UI components.
 */
class IProgressReporter {
public:
    virtual ~IProgressReporter() = default;

    /**
     * @brief Report progress update
     * @param progress Value in [0,1] for determinate progress, <0 for indeterminate
     * @param message Optional status message
     */
    virtual void reportProgress(double progress, const std::string& message) = 0;

    /**
     * @brief Report an error during operation
     * @param error Error description
     */
    virtual void reportError(const std::string& error) = 0;

    /**
     * @brief Check if operation was cancelled by user
     * @return true if cancellation was requested
     */
    virtual bool isCancelled() const = 0;
};

using ProgressReporterPtr = std::shared_ptr<IProgressReporter>;

/**
 * @brief Register all available service components
 */
void registerServices();

/**
 * @brief Base interface for all backend service components
 *
 * Services handle specific domain operations (e.g., IO, mesh processing).
 * Each service processes requests identified by action_id strings.
 */
class ServiceBase {
public:
    ServiceBase() = default;
    virtual ~ServiceBase() = default;

    /**
     * @brief Process a service request
     * @param action_id Identifier for the specific action to perform
     * @param params JSON parameters for the action
     * @param reporter Optional progress reporter for long operations
     * @return JSON result object with operation outcome
     */
    virtual nlohmann::json processRequest(const std::string& action_id,
                                          const nlohmann::json& params,
                                          ProgressReporterPtr reporter) = 0;
};

/**
 * @brief Factory for creating new service instances
 */
class ServiceBaseFactory : public Kangaroo::Util::FactoryTraits<ServiceBaseFactory, ServiceBase> {
public:
    ServiceBaseFactory() = default;
    virtual ~ServiceBaseFactory() = default;

    virtual tObjectPtr create() const = 0;
};

/**
 * @brief Factory for singleton service instances
 *
 * Use for stateless services or services that manage shared resources.
 */
class ServiceBaseSingletonFactory
    : public Kangaroo::Util::FactoryTraits<ServiceBaseSingletonFactory, ServiceBase> {
public:
    ServiceBaseSingletonFactory() = default;
    virtual ~ServiceBaseSingletonFactory() = default;

    virtual tObjectSharedPtr instance() const = 0;
};

} // namespace App
} // namespace OpenGeoLab