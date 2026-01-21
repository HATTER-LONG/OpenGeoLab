#pragma once

#include "io/reader.hpp"
#include <kangaroo/util/component_factory.hpp>

namespace OpenGeoLab::IO {
class BrepReader : public ReaderBase {
public:
    BrepReader() = default;
    ~BrepReader() override = default;

    ReadResult readFile(const std::string& file_path,
                        Util::ProgressCallback progress_callback) override;
};

class BrepReaderFactory : public ReaderFactory {
public:
    BrepReaderFactory() = default;
    ~BrepReaderFactory() = default;

    tObjectPtr create() override { return std::make_unique<BrepReader>(); };
};
} // namespace OpenGeoLab::IO