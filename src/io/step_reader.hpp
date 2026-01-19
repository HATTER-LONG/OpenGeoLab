#pragma once

#include "io/reader.hpp"
#include <kangaroo/util/component_factory.hpp>

namespace OpenGeoLab::IO {
class StepReader : public ReaderBase {
public:
    StepReader() = default;
    ~StepReader() override = default;

    void readFile(const std::string& file_path) override;
};

class StepReaderFactory : public ReaderFactory {
public:
    StepReaderFactory() = default;
    ~StepReaderFactory() = default;

    tObjectPtr create() override { return std::make_unique<StepReader>(); };
};
} // namespace OpenGeoLab::IO