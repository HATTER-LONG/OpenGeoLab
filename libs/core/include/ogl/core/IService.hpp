/**
 * @file IService.hpp
 * @brief Service response and component service factory contracts for OpenGeoLab.
 */

#pragma once

#include <ogl/core/export.hpp>

#include <kangaroo/util/factorytraits.hpp>
#include <nlohmann/json.hpp>

#include <string>

namespace ogl::core {

/**
 * @brief Structured response returned by component-backed service calls.
 */
struct OGL_CORE_EXPORT ServiceResponse {
    bool success{false};
    std::string moduleName;
    std::string operationName;
    std::string message;
    nlohmann::json payload = nlohmann::json::object();

    /**
     * @brief Convert the response into a JSON payload suitable for QML and Python bridges.
     * @return Serialized response object.
     */
    [[nodiscard]] auto toJson() const -> nlohmann::json {
        return {
            {"success", success}, {"moduleName", moduleName}, {"operationName", operationName},
            {"message", message}, {"payload", payload},
        };
    }
};

/**
 * @brief Runtime service contract used behind Kangaroo component factories.
 */
class OGL_CORE_EXPORT IService {
public:
    virtual ~IService() = default;

    /**
     * @brief Execute a service request for a logical module.
     * @param module_name Module identifier such as geometry or mesh.
     * @param params Parsed request payload.
     * @return Structured service response.
     */
    virtual auto processRequest(const std::string& module_name, const nlohmann::json& params)
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

} // namespace ogl::core