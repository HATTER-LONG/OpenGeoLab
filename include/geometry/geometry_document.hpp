#pragma once

#include "entity_index.hpp"

#include <kangaroo/util/noncopyable.hpp>

#include <memory>
#include <string>
#include <vector>

class TopoDS_Shape;

namespace OpenGeoLab::Geometry {

/**
 * @brief A geometry document holds one imported/created model.
 *
 * Responsibilities:
 * - Own the root entity (typically a Compound)
 * - Maintain an EntityIndex for fast lookups
 * - Expose "parts" (UI-level independent components) as root children
 */
class GeometryDocument : public Kangaroo::Util::NonCopyMoveable {
public:
    using Ptr = std::shared_ptr<GeometryDocument>;

    GeometryDocument() = default;
    explicit GeometryDocument(std::string name);
    ~GeometryDocument() = default;

    void clear();

    [[nodiscard]] bool setRootEntity(const GeometryEntityPtr& root);
    [[nodiscard]] GeometryEntityPtr rootEntity() const { return m_root; }

    [[nodiscard]] const std::string& name() const { return m_name; }
    void setName(std::string name);

    [[nodiscard]] const std::string& sourcePath() const { return m_sourcePath; }
    void setSourcePath(std::string source_path);

    [[nodiscard]] EntityIndex& index() { return m_index; }
    [[nodiscard]] const EntityIndex& index() const { return m_index; }
    void rebuildIndex();

    [[nodiscard]] GeometryEntityPtr findById(EntityId entity_id) const;
    [[nodiscard]] GeometryEntityPtr findByUIDAndType(EntityUID entity_uid,
                                                     EntityType entity_type) const;
    [[nodiscard]] GeometryEntityPtr findByShape(const TopoDS_Shape& shape) const;

    [[nodiscard]] std::vector<GeometryEntityPtr> parts() const;

private:
    std::string m_name;
    std::string m_sourcePath;

    GeometryEntityPtr m_root;
    EntityIndex m_index;
};

} // namespace OpenGeoLab::Geometry