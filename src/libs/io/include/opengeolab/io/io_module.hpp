/**
 * @file io_module.hpp
 * @brief IOModule — reading and writing geometry files
 *
 * Request format: {"module": "io", "action": "<name>", "param": {...}}
 */

#pragma once

#include <opengeolab/core/module.hpp>
#include <opengeolab/io/io_export.hpp>

namespace Kangaroo::Util {
class PluginComponentFactory;
} // namespace Kangaroo::Util

namespace OpenGeoLab::IO {

/**
 * @brief I/O module — delegates to factory-managed IAction singletons.
 *
 * Actions are registered during construction via registerAction<T>().
 */
class OPENGEOLAB_IO_EXPORT IOModule final : public Core::ModuleBase {
public:
    explicit IOModule(Kangaroo::Util::PluginComponentFactory& factory);
    ~IOModule() override;

    static constexpr std::string_view MODULE_NAME{"io"};
};

} // namespace OpenGeoLab::IO
