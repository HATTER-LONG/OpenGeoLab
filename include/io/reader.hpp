#pragma once
#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/noncopyable.hpp>
#include <string>

namespace OpenGeoLab::IO {
class ReaderBase : public Kangaroo::Util::NonCopyMoveable {
public:
    ReaderBase() = default;
    virtual ~ReaderBase() = default;

    virtual void readFile(const std::string& file_path) = 0;
};

class ReaderFactory : public Kangaroo::Util::FactoryTraits<ReaderFactory, ReaderBase> {
public:
    ReaderFactory() = default;
    ~ReaderFactory() = default;

    virtual tObjectPtr create() = 0;
};
} // namespace OpenGeoLab::IO