#pragma once

#include "util/progress_callback.hpp"
#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/noncopyable.hpp>
#include <nlohmann/json.hpp>

namespace OpenGeoLab::Render {
class RenderActionBase : public Kangaroo::Util::NonCopyMoveable {
public:
    RenderActionBase() = default;
    virtual ~RenderActionBase() = default;

    /**
     * @brief Execute the render action
     * @param params Action-specific JSON parameters
     * @param progress_callback Optional callback for progress reporting
     * @return JSON result object
     * @note Implementations should report progress through the callback
     *       and return error information in the JSON result on failure.
     */
    [[nodiscard]] virtual nlohmann::json execute(const nlohmann::json& params,
                                                 Util::ProgressCallback progress_callback) = 0;
};

class RenderActionFactory
    : public Kangaroo::Util::FactoryTraits<RenderActionFactory, RenderActionBase> {
public:
    RenderActionFactory() = default;
    ~RenderActionFactory() = default;

    virtual tObjectPtr create() = 0;
};
} // namespace OpenGeoLab::Render