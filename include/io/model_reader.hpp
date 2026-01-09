#pragma once

#include "app/service.hpp"

#include <kangaroo/util/component_factory.hpp>
#include <nlohmann/json.hpp>

namespace OpenGeoLab::IO {
class ModelReader : public App::IService {
public:
    ModelReader() = default;
    ~ModelReader() override = default;

    nlohmann::json processRequest(const std::string& module_name,
                                  const nlohmann::json& params,
                                  App::IProgressReporterPtr progress_reporter) override;
};

class ModelReaderFactory : public App::IServiceSigletonFactory {
public:
    ModelReaderFactory() = default;
    ~ModelReaderFactory() override = default;

    tObjectSharedPtr instance() const override;
};
} // namespace OpenGeoLab::IO