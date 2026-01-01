#pragma once

#include "io/reader.hpp"

#include <kangaroo/util/component_factory.hpp>

#include <memory>
#include <string>

namespace OpenGeoLab {
namespace IO {

class BrepReader : public ReaderBase {
public:
    BrepReader() = default;
    virtual ~BrepReader() = default;

    virtual GeometryDataPtr read(const std::string& file_path) override;

private:
    bool parseBrepFile(const std::string& file_path, GeometryData& data);
};

class BrepReaderFactory : public ReaderFactory {
public:
    BrepReaderFactory() = default;
    virtual ~BrepReaderFactory() = default;

    virtual tObjectPtr create() override { return std::make_unique<BrepReader>(); }
};

} // namespace IO
} // namespace OpenGeoLab
