#include <opengeolab/render/batch_utils.hpp>

namespace OpenGeoLab::Render::BatchUtils {

void multiDrawElements(GLenum mode, const IndexedBatch& batch) {
    if (batch.drawCount() == 0)
        return;
    glMultiDrawElements(mode,
                        batch.counts.data(),
                        GL_UNSIGNED_INT,
                        batch.offsets.data(),
                        batch.drawCount());
}

void multiDrawArrays(GLenum mode, const ArrayBatch& batch) {
    if (batch.drawCount() == 0)
        return;
    glMultiDrawArrays(mode, batch.firsts.data(), batch.counts.data(), batch.drawCount());
}

} // namespace OpenGeoLab::Render::BatchUtils
