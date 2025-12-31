#pragma once

#include "service.hpp"
#include <kangaroo/util/component_factory.hpp>
#include <string>

namespace OpenGeoLab {
namespace IO {

class IModelReader : public App::ServiceBase {
public:
    IModelReader() = default;
    virtual ~IModelReader() = default;

    virtual bool processRequest(const std::string& action_id,
                                const nlohmann::json& params) override;

private:
    bool readModel(const std::string& file_path);
};

class ModelReaderFactory : public App::ServiceBaseSingletonFactory {
public:
    ModelReaderFactory() = default;
    virtual ~ModelReaderFactory() = default;

    virtual tObjectSharedPtr instance() const override;
};
} // namespace IO
} // namespace OpenGeoLab