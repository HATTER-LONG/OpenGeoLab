/**
 * @file step_reader.hpp
 * @brief STEP format geometry reader component
 */
#pragma once

#include "io/reader.hpp"

#include <kangaroo/util/component_factory.hpp>

#include <memory>
#include <string>

namespace OpenGeoLab {
namespace IO {

class StepReader : public ReaderBase {
public:
    StepReader() = default;
    virtual ~StepReader() = default;

    virtual GeometryDataPtr read(const std::string& file_path) override;

private:
    bool parseStepFile(const std::string& file_path, GeometryData& data);
};

class StepReaderFactory : public ReaderFactory {
public:
    StepReaderFactory() = default;
    virtual ~StepReaderFactory() = default;

    virtual tObjectPtr create() override { return std::make_unique<StepReader>(); }
};

} // namespace IO
} // namespace OpenGeoLab
