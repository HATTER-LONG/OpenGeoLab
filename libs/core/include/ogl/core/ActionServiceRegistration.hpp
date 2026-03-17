/**
 * @file ActionServiceRegistration.hpp
 * @brief Shared action-service registration helpers for module-backed request dispatch.
 */

#pragma once

#include <ogl/core/ActionExecutionUtilities.hpp>
#include <ogl/core/IService.hpp>

#include <kangaroo/util/component_factory.hpp>

#include <algorithm>
#include <cctype>
#include <functional>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace OGL::Core {

/**
 * @brief Registration inputs for a module service that dispatches pluggable action handlers.
 * @tparam ActionInterface Action contract executed by the service.
 * @tparam LoggerHooks Logger adapter that records registration and dispatch lifecycle events.
 */
template <class ActionInterface, class LoggerHooks> struct ActionServiceRegistrationSpec {
    std::string moduleName;
    std::vector<std::string> supportedActions;
    std::function<std::shared_ptr<ActionInterface>(const std::string&)> createActionById;
    LoggerHooks loggerHooks;
};

namespace Detail {

inline auto capitalizeModuleName(std::string moduleName) -> std::string {
    if(!moduleName.empty()) {
        moduleName.front() =
            static_cast<char>(std::toupper(static_cast<unsigned char>(moduleName.front())));
    }
    return moduleName;
}

inline auto sortSupportedActions(std::vector<std::string> actionNames) -> std::vector<std::string> {
    std::sort(actionNames.begin(), actionNames.end());
    return actionNames;
}

inline auto summarizeSupportedActions(const std::vector<std::string>& supportedActions)
    -> std::string {
    const auto sortedActions = sortSupportedActions(supportedActions);

    std::ostringstream stream;
    for(std::size_t index = 0; index < sortedActions.size(); ++index) {
        if(index > 0) {
            stream << ", ";
        }
        stream << sortedActions[index];
    }

    return stream.str();
}

template <class ActionInterface, class LoggerHooks>
inline void validateActionServiceRegistrationSpec(
    const ActionServiceRegistrationSpec<ActionInterface, LoggerHooks>& spec) {
    if(spec.moduleName.empty()) {
        throw std::invalid_argument(
            "Action service registration requires a non-empty module name.");
    }

    if(spec.supportedActions.empty()) {
        throw std::invalid_argument("Action service registration for module '" + spec.moduleName +
                                    "' requires at least one supported action.");
    }

    if(!spec.createActionById) {
        throw std::invalid_argument("Action service registration for module '" + spec.moduleName +
                                    "' requires a createActionById callback.");
    }

    const auto sortedActions = sortSupportedActions(spec.supportedActions);
    const auto duplicate = std::adjacent_find(sortedActions.begin(), sortedActions.end());
    if(duplicate != sortedActions.end()) {
        throw std::invalid_argument("Duplicate action ids are not allowed: " + *duplicate);
    }

    const auto emptyActionId = std::find(sortedActions.begin(), sortedActions.end(), "");
    if(emptyActionId != sortedActions.end()) {
        throw std::invalid_argument("Supported action ids must be non-empty strings.");
    }
}

inline auto buildModuleMismatchResponse(const ServiceRequest& request, std::string_view moduleName)
    -> ServiceResponse {
    return {.success = false,
            .module = request.module,
            .action = request.action,
            .message = capitalizeModuleName(std::string{moduleName}) +
                       " service only accepts the " + std::string{moduleName} + " module.",
            .payload = nlohmann::json::object()};
}

inline auto buildUnsupportedActionResponse(const ServiceRequest& request,
                                           std::string_view moduleName,
                                           std::string_view supportedActionSummary)
    -> ServiceResponse {
    return {.success = false,
            .module = request.module,
            .action = request.action,
            .message = "Unsupported " + std::string{moduleName} +
                       " action. Registered actions: " + std::string{supportedActionSummary} + ".",
            .payload = nlohmann::json::object()};
}

inline auto buildNullFactoryResponse(const ServiceRequest& request, std::string_view moduleName)
    -> ServiceResponse {
    return buildFailureResponse(request, capitalizeModuleName(std::string{moduleName}) +
                                             " action factory resolved a null action instance.");
}

} // namespace Detail

/**
 * @brief Shared service skeleton that validates module/action routing before invoking actions.
 * @tparam ActionInterface Action contract executed by the service.
 * @tparam LoggerHooks Logger adapter used by the service.
 */
template <class ActionInterface, class LoggerHooks> class ActionService final : public IService {
public:
    explicit ActionService(ActionServiceRegistrationSpec<ActionInterface, LoggerHooks> spec)
        : m_spec(std::move(spec)),
          m_sortedSupportedActions(Detail::sortSupportedActions(m_spec.supportedActions)),
          m_supportedActionSummary(Detail::summarizeSupportedActions(m_spec.supportedActions)) {
        Detail::validateActionServiceRegistrationSpec(m_spec);
    }

    auto processRequest(const ServiceRequest& request, const ProgressCallback& progressCallback)
        -> ServiceResponse override {
        if(request.module != m_spec.moduleName) {
            return Detail::buildModuleMismatchResponse(request, m_spec.moduleName);
        }

        if(!std::binary_search(m_sortedSupportedActions.begin(), m_sortedSupportedActions.end(),
                               request.action)) {
            return Detail::buildUnsupportedActionResponse(request, m_spec.moduleName,
                                                          m_supportedActionSummary);
        }

        try {
            auto action = m_spec.createActionById(request.action);
            if(!action) {
                m_spec.loggerHooks.onFactoryNull(m_spec.moduleName, request.action);
                return Detail::buildNullFactoryResponse(request, m_spec.moduleName);
            }

            m_spec.loggerHooks.onDispatch(m_spec.moduleName, request.action);
            return action->execute(request, progressCallback);
        } catch(const std::exception& ex) {
            m_spec.loggerHooks.onError(m_spec.moduleName, request.action, ex.what());
            return buildFailureResponse(request, ex.what());
        }
    }

private:
    ActionServiceRegistrationSpec<ActionInterface, LoggerHooks> m_spec;
    std::vector<std::string> m_sortedSupportedActions;
    std::string m_supportedActionSummary;
};

/**
 * @brief Shared singleton-factory wrapper that exposes an action service through Kangaroo.
 * @tparam ActionInterface Action contract executed by the service.
 * @tparam LoggerHooks Logger adapter used by the service.
 */
template <class ActionInterface, class LoggerHooks>
class ActionServiceFactory : public IServiceSingletonFactory {
public:
    explicit ActionServiceFactory(ActionServiceRegistrationSpec<ActionInterface, LoggerHooks> spec)
        : m_service(
              std::make_shared<ActionService<ActionInterface, LoggerHooks>>(std::move(spec))) {}

    auto instance() const -> tObjectSharedPtr override { return m_service; }

private:
    std::shared_ptr<ActionService<ActionInterface, LoggerHooks>> m_service;
};

/**
 * @brief Register a shared action service factory and emit the standardized registration log.
 * @tparam ServiceFactory Concrete factory type, usually ActionServiceFactory specialization.
 * @tparam ActionInterface Action contract executed by the service.
 * @tparam LoggerHooks Logger adapter used by the service.
 * @param spec Module registration inputs.
 */
template <class ServiceFactory, class ActionInterface, class LoggerHooks>
inline void
registerActionService(const ActionServiceRegistrationSpec<ActionInterface, LoggerHooks>& spec) {
    Detail::validateActionServiceRegistrationSpec(spec);

    g_ComponentFactory.registInstanceFactoryWithID<ServiceFactory>(spec.moduleName, spec);
    spec.loggerHooks.onRegistered(spec.moduleName,
                                  Detail::summarizeSupportedActions(spec.supportedActions));
}

} // namespace OGL::Core
