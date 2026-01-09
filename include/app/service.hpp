#pragma once

#include <kangaroo/util/component_factory.hpp>
#include <nlohmann/json.hpp>

namespace OpenGeoLab::App {
class IProgressReporter {
public:
    virtual ~IProgressReporter() = default;

    virtual void reportProgress(double progress, const std::string& message) = 0;

    virtual void reportError(const std::string& error_message) = 0;

    virtual bool isCancelled() const = 0;
};

using IProgressReporterPtr = std::shared_ptr<IProgressReporter>;

class IService {
public:
    IService() = default;
    virtual ~IService() = default;

    virtual nlohmann::json processRequest(const std::string& module_name,
                                          const nlohmann::json& params,
                                          IProgressReporterPtr progress_reporter) = 0;
};
class IServiceFactory : public Kangaroo::Util::FactoryTraits<IServiceFactory, IService> {
public:
    IServiceFactory() = default;
    virtual ~IServiceFactory() = default;

    virtual tObjectPtr create() const = 0;
};

class IServiceSigletonFactory
    : public Kangaroo::Util::FactoryTraits<IServiceSigletonFactory, IService> {
public:
    IServiceSigletonFactory() = default;
    virtual ~IServiceSigletonFactory() = default;

    virtual tObjectSharedPtr instance() const = 0;
};

void registerServices();
} // namespace OpenGeoLab::App