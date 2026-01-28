#pragma once

#include "util/occ_progress.hpp"

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/noncopyable.hpp>
#include <nlohmann/json.hpp>

namespace OpenGeoLab::Geometry {
class GeometryActionBase : public Kangaroo::Util::NonCopyMoveable {
public:
    GeometryActionBase() = default;
    virtual ~GeometryActionBase() = default;

    [[nodiscard]] virtual bool execute(const nlohmann::json& params,
                                       Util::ProgressCallback progress_callback) = 0;
};

class GeometryActionFactory
    : public Kangaroo::Util::FactoryTraits<GeometryActionFactory, GeometryActionBase> {
public:
    GeometryActionFactory() = default;
    ~GeometryActionFactory() = default;

    virtual tObjectPtr create() = 0;
};

} // namespace OpenGeoLab::Geometry