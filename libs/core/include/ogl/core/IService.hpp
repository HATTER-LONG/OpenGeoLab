/**
 * @file IService.hpp
 * @brief Service response and component service factory contracts for OpenGeoLab.
 */

#pragma once

#include <ogl/core/export.hpp>

#include <kangaroo/util/factorytraits.hpp>
#include <nlohmann/json.hpp>

#include <functional>
#include <string>

namespace OGL::Core {

/**
 * @brief Structured request routed through a module service and action handler.
 */
struct OGL_CORE_EXPORT ServiceRequest {
    std::string module;
    std::string action;
    nlohmann::json param = nlohmann::json::object();

    /**
     * @brief Convert the request into a JSON payload suitable for QML and Python bridges.
     * @return Serialized request object.
     */
    [[nodiscard]] auto toJson() const -> nlohmann::json {
        return {{"module", module}, {"action", action}, {"param", param}};
    }
};

/**
 * @brief Structured response returned by component-backed service calls.
 */
struct OGL_CORE_EXPORT ServiceResponse {
    bool success{false};
    std::string module;
    std::string action;
    std::string message;
    nlohmann::json payload = nlohmann::json::object();

    /**
     * @brief Convert the response into a JSON payload suitable for QML and Python bridges.
     * @return Serialized response object.
     */
    [[nodiscard]] auto toJson() const -> nlohmann::json {
        return {
            {"success", success}, {"module", module},   {"action", action},
            {"message", message}, {"payload", payload},
        };
    }
};

/**
 * @brief Progress callback used by services and actions to report intermediate work.
 * @param progress Progress value in the range [0.0, 1.0].
 * @param message Human-readable progress message.
 * @return False when the operation should stop early.
 */
using ProgressCallback = std::function<bool(double progress, const std::string& message)>;

/**
 * @brief Runtime service contract used behind Kangaroo component factories.
 */
class OGL_CORE_EXPORT IService {
public:
    virtual ~IService() = default;

    /**
     * @brief Execute a service request for a logical module and action pair.
     * @param request Parsed request containing module, action, and param payload.
     * @param progress_callback Optional callback for intermediate progress updates.
     * @return Structured service response.
     */
    virtual auto processRequest(const ServiceRequest& request,
                                const ProgressCallback& progress_callback = {})
        -> ServiceResponse = 0;
};

/**
 * @brief Kangaroo instance-factory contract for singleton-like OpenGeoLab services.
 */
class OGL_CORE_EXPORT IServiceSingletonFactory
    : public Kangaroo::Util::FactoryTraits<IServiceSingletonFactory, IService> {
public:
    virtual ~IServiceSingletonFactory() = default;

    /**
     * @brief Return the shared service instance associated with this factory.
     * @return Shared service object.
     */
    virtual auto instance() const -> tObjectSharedPtr = 0;
};

} // namespace OGL::Core
