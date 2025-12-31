#pragma once

#include <kangaroo/util/component_factory.hpp>
#include <nlohmann/json.hpp>
#include <string>

namespace OpenGeoLab {
namespace App {

void registeServices();

class ServiceBase {
public:
    ServiceBase() = default;
    virtual ~ServiceBase() = default;

    virtual bool processRequest(const std::string& action_id, const nlohmann::json& params) = 0;
};

class ServiceBaseFactory : public Kangaroo::Util::FactoryTraits<ServiceBaseFactory, ServiceBase> {
public:
    ServiceBaseFactory() = default;
    virtual ~ServiceBaseFactory() = default;

    virtual tObjectPtr create() const = 0;
};

class ServiceBaseSingletonFactory
    : public Kangaroo::Util::FactoryTraits<ServiceBaseSingletonFactory, ServiceBase> {
public:
    ServiceBaseSingletonFactory() = default;
    virtual ~ServiceBaseSingletonFactory() = default;

    virtual tObjectSharedPtr instance() const = 0;
};
} // namespace App
} // namespace OpenGeoLab