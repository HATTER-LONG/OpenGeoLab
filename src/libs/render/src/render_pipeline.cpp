/**
 * @file render_pipeline.cpp
 * @brief Qt Rendering Hardware Interface implementation of RenderPipeline.
 */

#include <opengeolab/render/render_pipeline.hpp>

#include "pick_resolver.hpp"
#include "render_pipeline_detail.hpp"

#include <opengeolab/core/color_map.hpp>
#include <opengeolab/render/label_anchor.hpp>
#include <opengeolab/render/render_scene_snapshot.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <QFile>
#include <QFont>
#include <QFontMetricsF>
#include <QImage>
#include <QMatrix4x4>
#include <QPainter>
#include <QVector4D>
#include <rhi/qrhi.h>
#include <rhi/qshader.h>

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <vector>

namespace OpenGeoLab::Render {

namespace {

constexpr int K_UNIFORM_SLOT_COUNT = 6;
constexpr qsizetype K_INITIAL_VERTEX_BUFFER_SIZE = 4 * 1024 * 1024;
constexpr qsizetype K_INITIAL_INDEX_BUFFER_SIZE = 2 * 1024 * 1024;

struct alignas(16) UniformBlock {
    std::array<float, 16> mvp{};
    std::array<float, 16> view{};
    std::array<float, 4> tint{};
    std::array<float, 4> params{};
    std::array<float, 4> viewport{};
};
static_assert(sizeof(UniformBlock) == 176);

struct ScreenPoint {
    float x{};
    float y{};
    float depth{};
    bool valid{false};
};

[[nodiscard]] QShader loadShader(const QString& path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? QShader::fromSerialized(file.readAll()) : QShader{};
}

[[nodiscard]] QMatrix4x4 toQtMatrix(const glm::mat4& matrix) {
    QMatrix4x4 result;
    std::memcpy(result.data(), glm::value_ptr(matrix), sizeof(glm::mat4));
    return result;
}

[[nodiscard]] QVector4D toVector(const Core::RenderColor& color, float alpha_override = -1.0F) {
    return {color.r, color.g, color.b, alpha_override >= 0.0F ? alpha_override : color.a};
}

[[nodiscard]] bool isFace(Core::EntityType type) {
    return type == Core::EntityType::GeoFace || type == Core::EntityType::GeoSolid ||
           type == Core::EntityType::MeshElement;
}

[[nodiscard]] bool isEdge(Core::EntityType type) {
    return type == Core::EntityType::GeoEdge || type == Core::EntityType::GeoWire ||
           type == Core::EntityType::MeshEdge;
}

[[nodiscard]] bool isPoint(Core::EntityType type) {
    return type == Core::EntityType::GeoVertex || type == Core::EntityType::MeshNode;
}

[[nodiscard]] bool maskAccepts(Core::EntityType type, PickMask mask) {
    if(Detail::pickModeFromMask(mask) != PickMode::VEF) {
        return true;
    }
    return (Core::maskForEntityType(type) & mask) != PickMask::None;
}

[[nodiscard]] float
distanceToSegment(float px, float py, const ScreenPoint& a, const ScreenPoint& b) {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const float length_sq = dx * dx + dy * dy;
    if(length_sq <= 1.0e-8F) {
        return std::hypot(px - a.x, py - a.y);
    }
    const float t = std::clamp(((px - a.x) * dx + (py - a.y) * dy) / length_sq, 0.0F, 1.0F);
    return std::hypot(px - (a.x + t * dx), py - (a.y + t * dy));
}

[[nodiscard]] bool pointInTriangle(
    float px, float py, const ScreenPoint& a, const ScreenPoint& b, const ScreenPoint& c) {
    const float d1 = (px - b.x) * (a.y - b.y) - (a.x - b.x) * (py - b.y);
    const float d2 = (px - c.x) * (b.y - c.y) - (b.x - c.x) * (py - c.y);
    const float d3 = (px - a.x) * (c.y - a.y) - (c.x - a.x) * (py - a.y);
    return !((d1 < 0.0F || d2 < 0.0F || d3 < 0.0F) && (d1 > 0.0F || d2 > 0.0F || d3 > 0.0F));
}

} // namespace

struct RenderPipeline::Impl {
    RenderSceneSnapshot snapshot;
    std::unique_ptr<PickResolver> pickResolver;
    std::string fontAtlasDir;
    uint64_t resolverVersion{0};
    FrameState pickState{};
    bool pickStateValid{false};

    QRhi* rhi{nullptr};
    QRhiRenderPassDescriptor* renderPassDescriptor{nullptr};
    int sampleCount{1};
    int uniformStride{0};
    qsizetype vertexCapacity{0};
    qsizetype indexCapacity{0};
    qsizetype tessellationIndexCapacity{0};
    uint64_t uploadedVersion{std::numeric_limits<uint64_t>::max()};
    uint64_t tessellationVersion{std::numeric_limits<uint64_t>::max()};
    uint32_t uploadedLabelVersion{std::numeric_limits<uint32_t>::max()};
    uint64_t uploadedOverlaySceneVersion{std::numeric_limits<uint64_t>::max()};
    std::size_t uploadedHighlightHash{std::numeric_limits<std::size_t>::max()};
    bool uploadedLabelsVisible{false};
    bool uploadedOverlayXray{false};
    glm::mat4 uploadedLabelMvp{0.0F};
    float pipelineDpr{0.0F};
    QSize labelTextureSize;
    std::vector<uint32_t> tessellationIndices;

    std::unique_ptr<QRhiBuffer> vertexBuffer;
    std::unique_ptr<QRhiBuffer> indexBuffer;
    std::unique_ptr<QRhiBuffer> tessellationIndexBuffer;
    std::unique_ptr<QRhiBuffer> uniformBuffer;
    std::unique_ptr<QRhiShaderResourceBindings> srb;
    std::unique_ptr<QRhiTexture> labelTexture;
    std::unique_ptr<QRhiSampler> labelSampler;
    std::unique_ptr<QRhiBuffer> labelUniformBuffer;
    std::unique_ptr<QRhiShaderResourceBindings> labelSrb;
    std::unique_ptr<QRhiGraphicsPipeline> surfacePipeline;
    std::unique_ptr<QRhiGraphicsPipeline> xrayPipeline;
    std::unique_ptr<QRhiGraphicsPipeline> linePipeline;
    std::unique_ptr<QRhiGraphicsPipeline> pointPipeline;
    std::unique_ptr<QRhiGraphicsPipeline> selectionLinePipeline;
    std::unique_ptr<QRhiGraphicsPipeline> hoverLinePipeline;
    std::unique_ptr<QRhiGraphicsPipeline> labelPipeline;

    void releaseGpuResources() {
        labelPipeline.reset();
        hoverLinePipeline.reset();
        selectionLinePipeline.reset();
        pointPipeline.reset();
        linePipeline.reset();
        xrayPipeline.reset();
        surfacePipeline.reset();
        srb.reset();
        labelSrb.reset();
        labelUniformBuffer.reset();
        labelSampler.reset();
        labelTexture.reset();
        uniformBuffer.reset();
        tessellationIndexBuffer.reset();
        indexBuffer.reset();
        vertexBuffer.reset();
        vertexCapacity = 0;
        indexCapacity = 0;
        tessellationIndexCapacity = 0;
        renderPassDescriptor = nullptr;
        uploadedVersion = std::numeric_limits<uint64_t>::max();
        uploadedLabelVersion = std::numeric_limits<uint32_t>::max();
        uploadedOverlaySceneVersion = std::numeric_limits<uint64_t>::max();
        uploadedHighlightHash = std::numeric_limits<std::size_t>::max();
        uploadedLabelMvp = glm::mat4{0.0F};
        labelTextureSize = {};
    }

    void rebuildTessellationIndices() {
        if(tessellationVersion == snapshot.sceneVersion()) {
            return;
        }
        tessellationIndices.clear();
        const auto indices = snapshot.indices();
        for(const auto& range : snapshot.triangleRanges()) {
            for(uint32_t i = 0; i + 2 < range.indexCount; i += 3) {
                const size_t base = static_cast<size_t>(range.indexOffset + i);
                if(base + 2 >= indices.size()) {
                    break;
                }
                const uint32_t a = indices[base];
                const uint32_t b = indices[base + 1];
                const uint32_t c = indices[base + 2];
                tessellationIndices.insert(tessellationIndices.end(), {a, b, b, c, c, a});
            }
        }
        tessellationVersion = snapshot.sceneVersion();
    }

    [[nodiscard]] static qsizetype bufferCapacity(qsizetype required, qsizetype minimum) {
        qsizetype capacity = minimum;
        while(capacity < required && capacity <= std::numeric_limits<int>::max() / 2) {
            capacity *= 2;
        }
        return std::max(capacity, required);
    }

    void createBuffers() {
        vertexCapacity =
            bufferCapacity(snapshot.vertices().size_bytes(), K_INITIAL_VERTEX_BUFFER_SIZE);
        indexCapacity =
            bufferCapacity(snapshot.indices().size_bytes(), K_INITIAL_INDEX_BUFFER_SIZE);
        tessellationIndexCapacity = bufferCapacity(std::span{tessellationIndices}.size_bytes(),
                                                   K_INITIAL_INDEX_BUFFER_SIZE);
        // Scene contents change frequently. Dynamic buffers are the portable QRhi
        // choice here; Immutable buffers are intended for data uploaded once.
        vertexBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer,
                                          static_cast<int>(vertexCapacity)));
        indexBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::IndexBuffer,
                                         static_cast<int>(indexCapacity)));
        tessellationIndexBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::IndexBuffer,
                                                     static_cast<int>(tessellationIndexCapacity)));
        uniformStride = rhi->ubufAligned(sizeof(UniformBlock));
        uniformBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer,
                                           uniformStride * K_UNIFORM_SLOT_COUNT));
        vertexBuffer->create();
        indexBuffer->create();
        tessellationIndexBuffer->create();
        uniformBuffer->create();
        srb.reset(rhi->newShaderResourceBindings());
        srb->setBindings({QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(
            0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
            uniformBuffer.get(), sizeof(UniformBlock))});
        srb->create();
    }

    void growGeometryBuffersIfNeeded() {
        const qsizetype required_vertices =
            std::max<qsizetype>(snapshot.vertices().size_bytes(), 1);
        const qsizetype required_indices = std::max<qsizetype>(snapshot.indices().size_bytes(), 1);
        const qsizetype required_tessellation =
            std::max<qsizetype>(std::span{tessellationIndices}.size_bytes(), 1);
        bool resized = false;
        const auto grow = [&resized](QRhiBuffer* buffer, qsizetype& capacity, qsizetype required) {
            if(required <= capacity) {
                return;
            }
            capacity = bufferCapacity(required, capacity);
            buffer->setSize(static_cast<int>(capacity));
            buffer->create();
            resized = true;
        };
        grow(vertexBuffer.get(), vertexCapacity, required_vertices);
        grow(indexBuffer.get(), indexCapacity, required_indices);
        grow(tessellationIndexBuffer.get(), tessellationIndexCapacity, required_tessellation);
        if(resized) {
            uploadedVersion = std::numeric_limits<uint64_t>::max();
        }
    }

    std::unique_ptr<QRhiGraphicsPipeline>
    createPipeline(QRhiGraphicsPipeline::Topology topology,
                   bool lit,
                   bool blend,
                   bool depth_write,
                   float line_width = 1.0F,
                   const QString& shader_override = {}) const {
        auto pipeline = std::unique_ptr<QRhiGraphicsPipeline>(rhi->newGraphicsPipeline());
        const QString name = shader_override.isEmpty()
                                 ? (lit ? QStringLiteral("mesh") : QStringLiteral("simple"))
                                 : shader_override;
        const QString prefix = QStringLiteral(":/opengeolab/render/shaders/resource/shaders/");
        pipeline->setShaderStages({
            {QRhiShaderStage::Vertex, loadShader(prefix + name + QStringLiteral(".vert.qsb"))},
            {QRhiShaderStage::Fragment, loadShader(prefix + name + QStringLiteral(".frag.qsb"))},
        });
        QRhiVertexInputLayout layout;
        const bool point_instances = shader_override == QStringLiteral("point");
        layout.setBindings({QRhiVertexInputBinding(
            sizeof(Scene::RenderVertex), point_instances ? QRhiVertexInputBinding::PerInstance
                                                         : QRhiVertexInputBinding::PerVertex)});
        layout.setAttributes({
            {0, 0, QRhiVertexInputAttribute::Float3, offsetof(Scene::RenderVertex, position)},
            {0, 1, QRhiVertexInputAttribute::Float3, offsetof(Scene::RenderVertex, normal)},
            {0, 2, QRhiVertexInputAttribute::Float4, offsetof(Scene::RenderVertex, color)},
        });
        pipeline->setVertexInputLayout(layout);
        pipeline->setTopology(topology);
        pipeline->setDepthTest(true);
        pipeline->setDepthWrite(depth_write);
        pipeline->setDepthOp(QRhiGraphicsPipeline::LessOrEqual);
        pipeline->setSampleCount(sampleCount);
        pipeline->setLineWidth(line_width);
        if(blend) {
            QRhiGraphicsPipeline::TargetBlend target_blend;
            target_blend.enable = true;
            target_blend.srcColor = QRhiGraphicsPipeline::One;
            target_blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
            target_blend.srcAlpha = QRhiGraphicsPipeline::One;
            target_blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
            pipeline->setTargetBlends({target_blend});
        }
        pipeline->setShaderResourceBindings(srb.get());
        pipeline->setRenderPassDescriptor(renderPassDescriptor);
        pipeline->create();
        return pipeline;
    }

    void createPipelines(float dpr) {
        const auto& colors = Core::ColorMap::active();
        surfacePipeline = createPipeline(QRhiGraphicsPipeline::Triangles, true, false, true);
        xrayPipeline = createPipeline(QRhiGraphicsPipeline::Triangles, true, true, false);
        linePipeline = createPipeline(QRhiGraphicsPipeline::Lines, false, false, false,
                                      colors.defaultEdgeWidth * dpr);
        // Hardware point-size support is inconsistent across RHI backends.
        // The visible vertex markers are drawn by the overlay texture; keep this
        // conventional pipeline only as a harmless fallback for future passes.
        pointPipeline = createPipeline(QRhiGraphicsPipeline::Points, false, true, false);
        selectionLinePipeline = createPipeline(QRhiGraphicsPipeline::Lines, false, false, false,
                                               colors.selectionEdgeVertex.lineWidth * dpr);
        hoverLinePipeline = createPipeline(QRhiGraphicsPipeline::Lines, false, false, false,
                                           colors.hoverEdgeVertex.lineWidth * dpr);
        pipelineDpr = dpr;
    }

    void createLabelResources(const QSize& size) {
        labelPipeline.reset();
        labelSrb.reset();
        labelUniformBuffer.reset();
        labelSampler.reset();
        labelTexture.reset();

        labelTexture.reset(rhi->newTexture(QRhiTexture::RGBA8, size, 1));
        labelTexture->create();
        labelSampler.reset(rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear,
                                           QRhiSampler::None, QRhiSampler::ClampToEdge,
                                           QRhiSampler::ClampToEdge));
        labelSampler->create();
        labelUniformBuffer.reset(
            rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 16 * sizeof(float)));
        labelUniformBuffer->create();
        labelSrb.reset(rhi->newShaderResourceBindings());
        labelSrb->setBindings(
            {QRhiShaderResourceBinding::sampledTexture(0, QRhiShaderResourceBinding::FragmentStage,
                                                       labelTexture.get(), labelSampler.get()),
             QRhiShaderResourceBinding::uniformBuffer(1, QRhiShaderResourceBinding::VertexStage,
                                                      labelUniformBuffer.get())});
        labelSrb->create();

        labelPipeline.reset(rhi->newGraphicsPipeline());
        const QString prefix = QStringLiteral(":/opengeolab/render/shaders/resource/shaders/");
        labelPipeline->setShaderStages({
            {QRhiShaderStage::Vertex, loadShader(prefix + QStringLiteral("label.vert.qsb"))},
            {QRhiShaderStage::Fragment, loadShader(prefix + QStringLiteral("label.frag.qsb"))},
        });
        labelPipeline->setVertexInputLayout({});
        labelPipeline->setTopology(QRhiGraphicsPipeline::Triangles);
        labelPipeline->setDepthTest(false);
        labelPipeline->setDepthWrite(false);
        labelPipeline->setSampleCount(sampleCount);
        QRhiGraphicsPipeline::TargetBlend blend;
        blend.enable = true;
        blend.srcColor = QRhiGraphicsPipeline::One;
        blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        blend.srcAlpha = QRhiGraphicsPipeline::One;
        blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        labelPipeline->setTargetBlends({blend});
        labelPipeline->setShaderResourceBindings(labelSrb.get());
        labelPipeline->setRenderPassDescriptor(renderPassDescriptor);
        labelPipeline->create();

        labelTextureSize = size;
        uploadedLabelVersion = std::numeric_limits<uint32_t>::max();
        uploadedOverlaySceneVersion = std::numeric_limits<uint64_t>::max();
        uploadedHighlightHash = std::numeric_limits<std::size_t>::max();
        uploadedLabelMvp = glm::mat4{0.0F};
    }

    [[nodiscard]] static QColor labelColor(const glm::vec4& color) {
        return QColor::fromRgbF(color.r, color.g, color.b, color.a);
    }

    [[nodiscard]] QImage buildLabelImage(const FrameState& state) const {
        QImage image(labelTextureSize, QImage::Format_RGBA8888_Premultiplied);
        image.fill(Qt::transparent);
        if(snapshot.pointRanges().empty() &&
           (!state.labelsVisible || state.resolvedLabels.empty())) {
            return image;
        }

        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        QFont font(QStringLiteral("Segoe UI"));
        font.setPixelSize(std::max(12, static_cast<int>(13.0F * state.devicePixelRatio)));
        font.setWeight(QFont::DemiBold);
        painter.setFont(font);
        const QFontMetricsF metrics(font);
        const glm::mat4 mvp = state.projMatrix * state.viewMatrix;
        const qreal padding_x = 7.0 * state.devicePixelRatio;
        const qreal padding_y = 4.0 * state.devicePixelRatio;
        const qreal marker_radius = 3.5 * state.devicePixelRatio;

        const auto project = [&](const glm::vec3& position) {
            ScreenPoint result;
            const glm::vec4 clip = mvp * glm::vec4(position, 1.0F);
            if(clip.w <= 1.0e-8F) {
                return result;
            }
            const glm::vec3 ndc = glm::vec3(clip) / clip.w;
            if(ndc.x < -1.0F || ndc.x > 1.0F || ndc.y < -1.0F || ndc.y > 1.0F || ndc.z < -1.0F ||
               ndc.z > 1.0F) {
                return result;
            }
            result.x = (ndc.x * 0.5F + 0.5F) * labelTextureSize.width();
            result.y = (0.5F - ndc.y * 0.5F) * labelTextureSize.height();
            result.depth = ndc.z;
            result.valid = true;
            return result;
        };

        std::vector<std::array<ScreenPoint, 3>> projected_triangles;
        projected_triangles.reserve(snapshot.indices().size() / 3);
        for(const auto& range : snapshot.triangleRanges()) {
            for(uint32_t i = 0; i + 2 < range.indexCount; i += 3) {
                const size_t index_offset = static_cast<size_t>(range.indexOffset) + i;
                if(index_offset + 2 >= snapshot.indices().size()) {
                    break;
                }
                std::array<ScreenPoint, 3> triangle;
                bool valid = true;
                for(size_t corner = 0; corner < triangle.size(); ++corner) {
                    const size_t vertex_index = snapshot.indices()[index_offset + corner];
                    if(vertex_index >= snapshot.vertices().size()) {
                        valid = false;
                        break;
                    }
                    const auto& position = snapshot.vertices()[vertex_index].position;
                    triangle[corner] = project({position[0], position[1], position[2]});
                    valid = valid && triangle[corner].valid;
                }
                if(valid) {
                    projected_triangles.push_back(triangle);
                }
            }
        }

        const auto is_occluded = [&](const glm::vec3& position) {
            if(state.xRayMode) {
                return false;
            }
            const ScreenPoint point = project(position);
            if(!point.valid) {
                return true;
            }
            constexpr float inside_epsilon = -1.0e-4F;
            constexpr float depth_epsilon = 8.0e-4F;
            for(const auto& triangle : projected_triangles) {
                const auto& a = triangle[0];
                const auto& b = triangle[1];
                const auto& c = triangle[2];
                const float denominator = (b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y);
                if(std::abs(denominator) <= 1.0e-7F) {
                    continue;
                }
                const float wa =
                    ((b.y - c.y) * (point.x - c.x) + (c.x - b.x) * (point.y - c.y)) / denominator;
                const float wb =
                    ((c.y - a.y) * (point.x - c.x) + (a.x - c.x) * (point.y - c.y)) / denominator;
                const float wc = 1.0F - wa - wb;
                if(wa >= inside_epsilon && wb >= inside_epsilon && wc >= inside_epsilon) {
                    const float surface_depth = wa * a.depth + wb * b.depth + wc * c.depth;
                    if(surface_depth < point.depth - depth_epsilon) {
                        return true;
                    }
                }
            }
            return false;
        };

        // Draw topology vertices and mesh nodes as stable screen-space markers.
        // This avoids backend-specific point-size behavior on D3D, Vulkan and Metal.
        for(const auto& range : snapshot.pointRanges()) {
            const auto matches = [&](const std::vector<HighlightEntry>& entries) {
                return std::ranges::any_of(entries, [&](const HighlightEntry& entry) {
                    return entry.range.shapeId == range.shapeId &&
                           entry.range.entityType == range.entityType &&
                           entry.range.localId == range.localId;
                });
            };
            const bool selected = matches(state.selectedEntries);
            const bool hovered = matches(state.hoveredEntries);
            const QColor fill = selected  ? QColor::fromRgb(255, 22, 93)
                                : hovered ? QColor::fromRgb(255, 127, 0)
                                : range.entityType == Core::EntityType::MeshNode
                                    ? QColor::fromRgb(45, 212, 191)
                                    : QColor::fromRgb(255, 82, 119);
            const qreal radius = ((range.entityType == Core::EntityType::MeshNode ? 3.8 : 5.0) +
                                  (selected || hovered ? 1.5 : 0.0)) *
                                 state.devicePixelRatio;
            for(uint32_t i = 0; i < range.vertexCount; ++i) {
                const size_t vertex_index = static_cast<size_t>(range.vertexOffset) + i;
                if(vertex_index >= snapshot.vertices().size()) {
                    continue;
                }
                const auto& position = snapshot.vertices()[vertex_index].position;
                const glm::vec3 world(position[0], position[1], position[2]);
                if(is_occluded(world)) {
                    continue;
                }
                const ScreenPoint screen = project(world);
                if(!screen.valid) {
                    continue;
                }
                const QPointF center(screen.x, screen.y);
                painter.setPen(QPen(QColor(15, 23, 42, 230),
                                    std::max<qreal>(1.0, 1.25 * state.devicePixelRatio)));
                painter.setBrush(fill);
                painter.drawEllipse(center, radius, radius);
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(255, 255, 255, 165));
                painter.drawEllipse(center - QPointF(radius * 0.28, radius * 0.28), radius * 0.28,
                                    radius * 0.28);
            }
        }

        if(state.labelsVisible) {
            for(const auto& label : state.resolvedLabels) {
                const ScreenPoint screen = project(label.anchorWorld);
                if(!screen.valid) {
                    continue;
                }
                painter.setOpacity(is_occluded(label.anchorWorld) ? 0.3 : 1.0);
                const qreal anchor_x = screen.x;
                const qreal anchor_y = screen.y;
                const QString text = QString::fromStdString(label.text);
                const qreal box_w = metrics.horizontalAdvance(text) + padding_x * 2.0;
                const qreal box_h = metrics.height() + padding_y * 2.0;
                qreal box_x = anchor_x + 9.0 * state.devicePixelRatio;
                qreal box_y = anchor_y - box_h - 7.0 * state.devicePixelRatio -
                              label.stackIndex * (box_h + 3.0 * state.devicePixelRatio);
                box_x =
                    std::clamp(box_x, 2.0, std::max(2.0, labelTextureSize.width() - box_w - 2.0));
                box_y =
                    std::clamp(box_y, 2.0, std::max(2.0, labelTextureSize.height() - box_h - 2.0));
                const QRectF box(box_x, box_y, box_w, box_h);
                const QColor accent = labelColor(label.textColor);
                QColor background = labelColor(label.bgColor);
                if(background.alphaF() < 0.72) {
                    background.setAlphaF(0.82);
                }

                const qreal stroke_width = std::max<qreal>(1.0, state.devicePixelRatio);
                painter.setPen(QPen(accent, stroke_width));
                painter.drawLine(QPointF(anchor_x, anchor_y), QPointF(box.left(), box.bottom()));
                painter.setBrush(accent);
                painter.drawEllipse(QPointF(anchor_x, anchor_y), marker_radius, marker_radius);
                painter.setPen(QPen(accent, stroke_width));
                painter.setBrush(background);
                painter.drawRoundedRect(box, 4.0 * state.devicePixelRatio,
                                        4.0 * state.devicePixelRatio);
                painter.setPen(accent.lighter(145));
                painter.drawText(box.adjusted(padding_x, padding_y, -padding_x, -padding_y),
                                 Qt::AlignCenter, text);
            }
            painter.setOpacity(1.0);
        }
        painter.end();
        return image;
    }

    void updateLabelTexture(QRhiResourceUpdateBatch* updates, const FrameState& state) {
        const QSize required_size(std::max(state.viewportWidth, 1),
                                  std::max(state.viewportHeight, 1));
        if(!labelTexture || labelTextureSize != required_size) {
            createLabelResources(required_size);
        }
        const QMatrix4x4 clip_correction = rhi->clipSpaceCorrMatrix();
        updates->updateDynamicBuffer(labelUniformBuffer.get(), 0, 16 * sizeof(float),
                                     clip_correction.constData());
        const glm::mat4 current_mvp = state.projMatrix * state.viewMatrix;
        std::size_t highlight_hash = 0;
        const auto hash_entries = [&highlight_hash](const std::vector<HighlightEntry>& entries,
                                                    std::size_t category) {
            const auto mix = [&highlight_hash](std::size_t value) {
                highlight_hash ^=
                    value + 0x9e3779b9U + (highlight_hash << 6U) + (highlight_hash >> 2U);
            };
            mix(category);
            for(const auto& entry : entries) {
                mix(std::hash<uint64_t>{}(entry.range.shapeId));
                mix(std::hash<uint32_t>{}(static_cast<uint32_t>(entry.range.entityType)));
                mix(std::hash<uint32_t>{}(entry.range.localId));
            }
        };
        hash_entries(state.selectedEntries, 1);
        hash_entries(state.hoveredEntries, 2);
        const bool camera_unchanged =
            std::memcmp(glm::value_ptr(uploadedLabelMvp), glm::value_ptr(current_mvp),
                        sizeof(glm::mat4)) == 0;
        if(uploadedLabelVersion == state.labelVersion &&
           uploadedLabelsVisible == state.labelsVisible && camera_unchanged &&
           uploadedOverlaySceneVersion == snapshot.sceneVersion() &&
           uploadedHighlightHash == highlight_hash && uploadedOverlayXray == state.xRayMode) {
            return;
        }
        const QImage image = buildLabelImage(state);
        QRhiTextureSubresourceUploadDescription subresource(image);
        QRhiTextureUploadDescription upload({QRhiTextureUploadEntry(0, 0, subresource)});
        updates->uploadTexture(labelTexture.get(), upload);
        uploadedLabelVersion = state.labelVersion;
        uploadedLabelsVisible = state.labelsVisible;
        uploadedLabelMvp = current_mvp;
        uploadedOverlaySceneVersion = snapshot.sceneVersion();
        uploadedHighlightHash = highlight_hash;
        uploadedOverlayXray = state.xRayMode;
    }

    void ensureResources(QRhi* current_rhi, QRhiRenderTarget* target, float dpr) {
        if(rhi != current_rhi) {
            releaseGpuResources();
            rhi = current_rhi;
        }
        if(renderPassDescriptor != target->renderPassDescriptor() ||
           sampleCount != target->sampleCount()) {
            releaseGpuResources();
            renderPassDescriptor = target->renderPassDescriptor();
            sampleCount = target->sampleCount();
        }
        if(!vertexBuffer) {
            createBuffers();
        }
        if(!surfacePipeline || std::abs(pipelineDpr - dpr) > 0.01F) {
            surfacePipeline.reset();
            xrayPipeline.reset();
            linePipeline.reset();
            pointPipeline.reset();
            selectionLinePipeline.reset();
            hoverLinePipeline.reset();
            createPipelines(dpr);
        }
    }

    [[nodiscard]] UniformBlock uniform(const FrameState& state,
                                       const Core::RenderColor& tint,
                                       float tint_amount,
                                       float alpha,
                                       float point_size,
                                       float depth_bias) const {
        UniformBlock block;
        const QMatrix4x4 mvp =
            rhi->clipSpaceCorrMatrix() * toQtMatrix(state.projMatrix * state.viewMatrix);
        const QMatrix4x4 view = toQtMatrix(state.viewMatrix);
        const QVector4D tint_vector = toVector(tint, tint_amount);
        std::copy_n(mvp.constData(), 16, block.mvp.begin());
        std::copy_n(view.constData(), 16, block.view.begin());
        block.tint = {tint_vector.x(), tint_vector.y(), tint_vector.z(), tint_vector.w()};
        block.params = {alpha, point_size, 0.0F, 0.0F};
        block.viewport = {static_cast<float>(state.viewportWidth),
                          static_cast<float>(state.viewportHeight), state.devicePixelRatio,
                          depth_bias};
        return block;
    }

    void bindUniform(QRhiCommandBuffer* cb, int slot) const {
        const QRhiCommandBuffer::DynamicOffset offset{0,
                                                      static_cast<quint32>(slot * uniformStride)};
        cb->setShaderResources(srb.get(), 1, &offset);
    }

    void bindGeometry(QRhiCommandBuffer* cb) const {
        const QRhiCommandBuffer::VertexInput binding(vertexBuffer.get(), 0);
        cb->setVertexInput(0, 1, &binding, indexBuffer.get(), 0, QRhiCommandBuffer::IndexUInt32);
    }

    void drawPointRange(QRhiCommandBuffer* cb, const Scene::DrawRange& range) const {
        const quint32 byte_offset =
            static_cast<quint32>(range.vertexOffset * sizeof(Scene::RenderVertex));
        const QRhiCommandBuffer::VertexInput binding(vertexBuffer.get(), byte_offset);
        cb->setVertexInput(0, 1, &binding);
        cb->draw(6, range.vertexCount);
    }

    void bindTessellationGeometry(QRhiCommandBuffer* cb) const {
        const QRhiCommandBuffer::VertexInput binding(vertexBuffer.get(), 0);
        cb->setVertexInput(0, 1, &binding, tessellationIndexBuffer.get(), 0,
                           QRhiCommandBuffer::IndexUInt32);
    }

    static void drawRange(QRhiCommandBuffer* cb, const Scene::DrawRange& range) {
        if(range.indexCount > 0) {
            cb->drawIndexed(range.indexCount, 1, range.indexOffset, 0);
        } else {
            cb->draw(range.vertexCount, 1, range.vertexOffset);
        }
    }

    [[nodiscard]] ScreenPoint project(size_t vertex_index) const {
        if(!pickStateValid || vertex_index >= snapshot.vertices().size()) {
            return {};
        }
        const auto& p = snapshot.vertices()[vertex_index].position;
        const glm::vec4 clip =
            pickState.projMatrix * pickState.viewMatrix * glm::vec4{p[0], p[1], p[2], 1.0F};
        if(clip.w <= 1.0e-8F) {
            return {};
        }
        const glm::vec3 ndc = glm::vec3{clip} / clip.w;
        return {(ndc.x * 0.5F + 0.5F) * static_cast<float>(pickState.viewportWidth),
                (0.5F - ndc.y * 0.5F) * static_cast<float>(pickState.viewportHeight), ndc.z,
                ndc.z >= -1.0F && ndc.z <= 1.0F};
    }

    [[nodiscard]] uint64_t pickIdForRange(const Scene::DrawRange& range) const {
        return range.vertexOffset < snapshot.pickIds().size()
                   ? snapshot.pickIds()[range.vertexOffset].pickId
                   : 0;
    }

    [[nodiscard]] float hitDistance(const Scene::DrawRange& range, float x, float y) const {
        constexpr float miss = std::numeric_limits<float>::infinity();
        if(isPoint(range.entityType)) {
            float best = miss;
            for(uint32_t i = 0; i < range.vertexCount; ++i) {
                const auto p = project(range.vertexOffset + i);
                if(p.valid) {
                    best = std::min(best, std::hypot(x - p.x, y - p.y));
                }
            }
            return best;
        }
        const auto indices = snapshot.indices();
        if(isEdge(range.entityType)) {
            float best = miss;
            for(uint32_t i = 0; i + 1 < range.indexCount; i += 2) {
                const size_t base = static_cast<size_t>(range.indexOffset + i);
                if(base + 1 >= indices.size()) {
                    break;
                }
                const auto a = project(indices[base]);
                const auto b = project(indices[base + 1]);
                if(a.valid && b.valid) {
                    best = std::min(best, distanceToSegment(x, y, a, b));
                }
            }
            return best;
        }
        if(isFace(range.entityType)) {
            for(uint32_t i = 0; i + 2 < range.indexCount; i += 3) {
                const size_t base = static_cast<size_t>(range.indexOffset + i);
                if(base + 2 >= indices.size()) {
                    break;
                }
                const auto a = project(indices[base]);
                const auto b = project(indices[base + 1]);
                const auto c = project(indices[base + 2]);
                if(a.valid && b.valid && c.valid && pointInTriangle(x, y, a, b, c)) {
                    return 0.0F;
                }
            }
        }
        return miss;
    }

    [[nodiscard]] std::vector<uint64_t> hits(float x, float y, float radius, PickMask mask) const {
        struct Hit {
            float distance;
            uint64_t id;
        };
        std::vector<Hit> found;
        const auto test = [&](std::span<const Scene::DrawRange> ranges) {
            for(const auto& range : ranges) {
                if(maskAccepts(range.entityType, mask)) {
                    const float distance = hitDistance(range, x, y);
                    if(distance <= radius) {
                        found.push_back({distance, pickIdForRange(range)});
                    }
                }
            }
        };
        test(snapshot.pointRanges());
        test(snapshot.lineRanges());
        test(snapshot.triangleRanges());
        std::ranges::sort(found, {}, &Hit::distance);
        std::vector<uint64_t> ids;
        for(const auto& hit : found) {
            if(hit.id != 0) {
                ids.push_back(hit.id);
            }
        }
        return ids;
    }
};

RenderPipeline::RenderPipeline() : m_impl(std::make_unique<Impl>()) {}
RenderPipeline::~RenderPipeline() = default;

void RenderPipeline::synchronize(const Scene::SceneGraph& scene) {
    if(scene.version() != m_impl->snapshot.sceneVersion()) {
        m_impl->snapshot.rebuild(scene);
    }
    if(scene.version() != m_impl->resolverVersion) {
        m_impl->pickResolver = std::make_unique<PickResolver>(scene.topologyIndex());
        m_impl->resolverVersion = scene.version();
    }
}

void RenderPipeline::render(QRhi* rhi,
                            QRhiCommandBuffer* cb,
                            QRhiRenderTarget* target,
                            const FrameState& state) {
    if(rhi == nullptr || cb == nullptr || target == nullptr) {
        return;
    }
    m_impl->rebuildTessellationIndices();
    m_impl->ensureResources(rhi, target, state.devicePixelRatio);
    m_impl->growGeometryBuffersIfNeeded();
    auto* updates = rhi->nextResourceUpdateBatch();
    m_impl->updateLabelTexture(updates, state);
    if(m_impl->uploadedVersion != m_impl->snapshot.sceneVersion()) {
        if(!m_impl->snapshot.empty()) {
            const auto vertices = m_impl->snapshot.vertices();
            updates->updateDynamicBuffer(m_impl->vertexBuffer.get(), 0,
                                         static_cast<quint32>(vertices.size_bytes()),
                                         vertices.data());
            if(!m_impl->snapshot.indices().empty()) {
                const auto indices = m_impl->snapshot.indices();
                updates->updateDynamicBuffer(m_impl->indexBuffer.get(), 0,
                                             static_cast<quint32>(indices.size_bytes()),
                                             indices.data());
            }
            if(!m_impl->tessellationIndices.empty()) {
                updates->updateDynamicBuffer(
                    m_impl->tessellationIndexBuffer.get(), 0,
                    static_cast<quint32>(std::span{m_impl->tessellationIndices}.size_bytes()),
                    m_impl->tessellationIndices.data());
            }
        }
        m_impl->uploadedVersion = m_impl->snapshot.sceneVersion();
    }

    const auto& colors = Core::ColorMap::active();
    const std::array<UniformBlock, K_UNIFORM_SLOT_COUNT> uniforms{
        m_impl->uniform(state, {}, 0.0F, state.xRayMode ? 0.25F : 1.0F,
                        colors.defaultPointSize * state.devicePixelRatio, 0.0F),
        m_impl->uniform(state, colors.defaultEdge, 0.0F, 1.0F,
                        colors.defaultPointSize * state.devicePixelRatio, 0.0003F),
        m_impl->uniform(state, colors.defaultVertex, 0.0F, 1.0F,
                        colors.defaultPointSize * 1.65F * state.devicePixelRatio, 0.0006F),
        m_impl->uniform(state, colors.selectionEdgeVertex.color, 1.0F, 1.0F,
                        colors.defaultPointSize * colors.selectionEdgeVertex.pointScale * 1.65F *
                            state.devicePixelRatio,
                        0.001F),
        m_impl->uniform(state, colors.hoverEdgeVertex.color, 1.0F, 1.0F,
                        colors.defaultPointSize * colors.hoverEdgeVertex.pointScale * 1.65F *
                            state.devicePixelRatio,
                        0.002F),
        m_impl->uniform(state, colors.defaultEdge, 1.0F, 0.55F, 4.0F * state.devicePixelRatio,
                        0.0008F),
    };
    for(int slot = 0; slot < K_UNIFORM_SLOT_COUNT; ++slot) {
        updates->updateDynamicBuffer(m_impl->uniformBuffer.get(), slot * m_impl->uniformStride,
                                     sizeof(UniformBlock), &uniforms[slot]);
    }

    cb->beginPass(target, QColor::fromRgbF(0.149F, 0.149F, 0.169F, 1.0F), {1.0F, 0}, updates);
    cb->setViewport({0.0F, 0.0F, static_cast<float>(state.viewportWidth),
                     static_cast<float>(state.viewportHeight)});
    if(!m_impl->snapshot.empty()) {
        using Scene::DisplayModeMask;
        if((state.displayMask & DisplayModeMask::Surface) != DisplayModeMask::None) {
            cb->setGraphicsPipeline(state.xRayMode ? m_impl->xrayPipeline.get()
                                                   : m_impl->surfacePipeline.get());
            m_impl->bindUniform(cb, 0);
            m_impl->bindGeometry(cb);
            for(const auto& range : m_impl->snapshot.triangleRanges()) {
                Impl::drawRange(cb, range);
            }
        }
        const auto draw_highlights = [&](const std::vector<HighlightEntry>& entries, int slot,
                                         QRhiGraphicsPipeline* edge_pipeline) {
            for(const auto& entry : entries) {
                // Point highlighting is part of the screen-space overlay below.
                if(isPoint(entry.entityType)) {
                    continue;
                }
                cb->setGraphicsPipeline(isFace(entry.entityType) ? m_impl->surfacePipeline.get()
                                                                 : edge_pipeline);
                m_impl->bindUniform(cb, slot);
                m_impl->bindGeometry(cb);
                Impl::drawRange(cb, entry.range);
            }
        };
        draw_highlights(state.selectedEntries, 3, m_impl->selectionLinePipeline.get());
        draw_highlights(state.hoveredEntries, 4, m_impl->hoverLinePipeline.get());
        if((state.displayMask & DisplayModeMask::Wireframe) != DisplayModeMask::None) {
            cb->setGraphicsPipeline(m_impl->linePipeline.get());
            m_impl->bindUniform(cb, 1);
            m_impl->bindGeometry(cb);
            for(const auto& range : m_impl->snapshot.lineRanges()) {
                Impl::drawRange(cb, range);
            }
        }
        if(state.showTessellation) {
            cb->setGraphicsPipeline(m_impl->linePipeline.get());
            m_impl->bindUniform(cb, 5);
            if(!m_impl->tessellationIndices.empty()) {
                m_impl->bindTessellationGeometry(cb);
                cb->drawIndexed(static_cast<quint32>(m_impl->tessellationIndices.size()));
            }
        }
    }
    const bool has_overlay = !m_impl->snapshot.pointRanges().empty() ||
                             (state.labelsVisible && !state.resolvedLabels.empty());
    if(has_overlay && m_impl->labelPipeline && m_impl->labelSrb) {
        cb->setGraphicsPipeline(m_impl->labelPipeline.get());
        cb->setShaderResources(m_impl->labelSrb.get());
        cb->draw(6);
    }
    cb->endPass();
    m_impl->pickState = state;
    m_impl->pickStateValid = true;
}

PickResult RenderPipeline::pickAt(int x, int y, PickMask mask) const {
    if(!m_impl->pickResolver || !m_impl->pickStateValid) {
        return {};
    }
    const float radius = 8.0F * m_impl->pickState.devicePixelRatio;
    return m_impl->pickResolver->resolve(
        m_impl->hits(static_cast<float>(x), static_cast<float>(y), radius, mask),
        Detail::pickModeFromMask(mask));
}

std::vector<PickResult>
RenderPipeline::pickRegion(int cx, int cy, int radius, PickMask mask) const {
    if(!m_impl->pickResolver || !m_impl->pickStateValid) {
        return {};
    }
    return m_impl->pickResolver->resolveAll(m_impl->hits(static_cast<float>(cx),
                                                         static_cast<float>(cy),
                                                         static_cast<float>(radius), mask),
                                            Detail::pickModeFromMask(mask));
}

std::vector<PickResult>
RenderPipeline::pickRect(int x0, int y0, int x1, int y1, PickMask mask) const {
    const int left = std::min(x0, x1);
    const int right = std::max(x0, x1);
    const int top = std::min(y0, y1);
    const int bottom = std::max(y0, y1);
    return pickRegion((left + right) / 2, (top + bottom) / 2,
                      static_cast<int>(std::hypot(right - left, bottom - top) * 0.5), mask);
}

void RenderPipeline::cleanup() {
    m_impl->releaseGpuResources();
    m_impl->rhi = nullptr;
    m_impl->pickResolver.reset();
    m_impl->pickStateValid = false;
}

std::span<const Scene::DrawRange> RenderPipeline::resolveEntityDrawRanges(
    uint32_t shape_id, Core::EntityType entity_type, uint32_t local_id) const {
    return m_impl->snapshot.lookupEntity(shape_id, entity_type, local_id);
}

std::vector<Scene::DrawRange> RenderPipeline::resolveShapeDrawRanges(uint32_t shape_id) const {
    std::vector<Scene::DrawRange> result;
    const auto append = [&](std::span<const Scene::DrawRange> ranges) {
        for(const auto& range : ranges) {
            if(range.shapeId == shape_id) {
                result.push_back(range);
            }
        }
    };
    append(m_impl->snapshot.triangleRanges());
    append(m_impl->snapshot.lineRanges());
    append(m_impl->snapshot.pointRanges());
    return result;
}

void RenderPipeline::setFontAtlasDir(const std::string& dir) { m_impl->fontAtlasDir = dir; }

glm::vec3 RenderPipeline::resolveEntityAnchor(uint32_t shape_id,
                                              Core::EntityType entity_type,
                                              uint32_t local_id) const {
    const auto ranges = m_impl->snapshot.lookupEntity(shape_id, entity_type, local_id);
    std::vector<glm::vec3> positions;
    for(const auto& range : ranges) {
        auto part = m_impl->snapshot.readVertexPositions(range.vertexOffset, range.vertexCount);
        positions.insert(positions.end(), part.begin(), part.end());
    }
    return entity_type == Core::EntityType::GeoEdge || entity_type == Core::EntityType::GeoWire
               ? computeAnchorMidpoint(positions)
               : computeAnchorFromVertices(positions);
}

} // namespace OpenGeoLab::Render
