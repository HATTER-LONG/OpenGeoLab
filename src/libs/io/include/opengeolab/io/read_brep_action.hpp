/**
 * @file read_brep_action.hpp
 * @brief ReadBrepAction — reads a BRep geometry file
 */

#pragma once

#include <opengeolab/core/action.hpp>
#include <opengeolab/io/io_export.hpp>

#include <string>

namespace OpenGeoLab::IO {

/**
 * @brief Action that reads a BRep (Boundary Representation) file.
 *
 * Expected param: {"path": "<file_path>"}
 * Returns: {"status": "ok", "path": "<file_path>", ...}
 */
class OPENGEOLAB_IO_EXPORT ReadBrepAction final : public Core::IAction {
public:
    ReadBrepAction();
    ~ReadBrepAction() override;

    [[nodiscard]] nlohmann::json describe() const override;

    [[nodiscard]] nlohmann::json execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) override;

    static constexpr std::string_view ACTION_NAME{"read_brep"};
};

} // namespace OpenGeoLab::IO
