#pragma once
#include "io/geometry_data.hpp"
#include <kangaroo/util/component_factory.hpp>

#include <string>

namespace OpenGeoLab {
namespace IO {
class ReaderBase {
public:
    ReaderBase() = default;
    virtual ~ReaderBase() = default;
    virtual GeometryDataPtr read(const std::string& file_path) = 0;
};

class ReaderFactory : public Kangaroo::Util::FactoryTraits<ReaderFactory, ReaderBase> {
public:
    ReaderFactory() = default;
    virtual ~ReaderFactory() = default;

    virtual tObjectPtr create() = 0;
};
} // namespace IO
} // namespace OpenGeoLab