#pragma once
#include <QOpenGLFunctions>
#include <QSize>

#include <kangaroo/util/noncopyable.hpp>

namespace OpenGeoLab::Render {

class RenderCore : protected QOpenGLFunctions, public Kangaroo::Util::NonCopyMoveable {
public:
    RenderCore();
    ~RenderCore();

    void initialize();

    bool isInitialized() const { return m_initialized; }

private:
    bool m_initialized{false};
};

} // namespace OpenGeoLab::Render