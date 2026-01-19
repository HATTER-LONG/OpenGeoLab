/**
 * @file service.hpp
 * @brief Core service interfaces for backend operations
 */

#pragma once

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/noncopyable.hpp>
#include <memory>
#include <nlohmann/json.hpp>


namespace OpenGeoLab::App {

/**
 * @brief Progress reporting interface for long-running operations
 *
 * Implementations bridge to UI or logging systems.
 */
class IProgressReporter : public Kangaroo::Util::NonCopyMoveable {
public:
    virtual ~IProgressReporter() = default;

    /**
     * @brief Report operation progress
     * @param progress Value in [0.0, 1.0]; negative for indeterminate
     * @param message Human-readable status message
     */
    virtual void reportProgress(double progress, const std::string& message) = 0;

    /// Report a non-fatal error message
    virtual void reportError(const std::string& error_message) = 0;

    /// Check if cancellation was requested
    virtual bool isCancelled() const = 0;
};

using IProgressReporterPtr = std::shared_ptr<IProgressReporter>;

/**
 * @brief Base interface for backend service modules
 */
class IService : public Kangaroo::Util::NonCopyMoveable {
public:
    IService() = default;
    virtual ~IService() = default;

    /**
     * @brief Execute a service request
     * @param module_name Service identifier
     * @param params Request parameters
     * @param progress_reporter Progress callback
     * @return JSON result data
     */
    virtual nlohmann::json processRequest(const std::string& module_name,
                                          const nlohmann::json& params,
                                          IProgressReporterPtr progress_reporter) = 0;
};

/**
 * @brief Factory interface for creating new service instances
 */
class IServiceFactory : public Kangaroo::Util::FactoryTraits<IServiceFactory, IService> {
public:
    IServiceFactory() = default;
    virtual ~IServiceFactory() = default;

    virtual tObjectPtr create() const = 0;
};

/**
 * @brief Factory interface for singleton service instances
 */
class IServiceSigletonFactory
    : public Kangaroo::Util::FactoryTraits<IServiceSigletonFactory, IService> {
public:
    IServiceSigletonFactory() = default;
    virtual ~IServiceSigletonFactory() = default;

    virtual tObjectSharedPtr instance() const = 0;
};

/// Register all built-in services with the component factory
void registerServices();

} // namespace OpenGeoLab::App