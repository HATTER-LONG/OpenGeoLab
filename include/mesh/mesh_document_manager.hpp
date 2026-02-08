/**
 * @file mesh_document_manager.hpp
 * @brief Mesh document manager interface for singleton access
 *
 * Provides a factory-based singleton to manage the current MeshDocument
 * instance, analogous to GeometryDocumentManager.
 */

#pragma once

#include "mesh/mesh_document.hpp"

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/noncopyable.hpp>
#include <memory>

namespace OpenGeoLab::Mesh {

/**
 * @brief Abstract manager for the active mesh document
 */
class MeshDocumentManager : public Kangaroo::Util::NonCopyMoveable {
public:
    virtual ~MeshDocumentManager() = default;

    /**
     * @brief Get the current mesh document (creates one lazily if needed)
     */
    [[nodiscard]] virtual MeshDocumentPtr currentDocument() = 0;

    /**
     * @brief Clear the current document and prepare a fresh one
     */
    [[nodiscard]] virtual MeshDocumentPtr newDocument() = 0;

protected:
    MeshDocumentManager() = default;
};

/**
 * @brief Singleton factory for MeshDocumentManager
 */
class IMeshDocumentManagerSingletonFactory
    : public Kangaroo::Util::FactoryTraits<IMeshDocumentManagerSingletonFactory,
                                           MeshDocumentManager> {
public:
    virtual tObjectSharedPtr instance() const = 0;
};

} // namespace OpenGeoLab::Mesh

/// Convenience macro to obtain the mesh document manager singleton
#define MeshDocumentMgrInstance                                                                    \
    g_ComponentFactory.getInstanceObject<Mesh::IMeshDocumentManagerSingletonFactory>()
