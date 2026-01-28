#pragma once

#include "geometry/geometry_action.hpp"

namespace OpenGeoLab::Geometry {
class CreateAction : public GeometryActionBase {
public:
    CreateAction() = default;
    ~CreateAction() = default;

    [[nodiscard]] static std::string actionName() { return "create"; }

    [[nodiscard]] bool execute(const nlohmann::json& params,
                               Util::ProgressCallback progress_callback) override;
};

class CreateActionFactory : public GeometryActionFactory {
public:
    CreateActionFactory() = default;
    ~CreateActionFactory() = default;

    tObjectPtr create() override { return std::make_unique<CreateAction>(); }
};
} // namespace OpenGeoLab::Geometry