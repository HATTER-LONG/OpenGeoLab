/**
 * @file mesh_document_manager_impl.hpp
 * @brief Concrete implementation of MeshDocumentManager
 */

#pragma once

#include "mesh/mesh_document_manager.hpp"
#include "mesh_document_impl.hpp"

namespace OpenGeoLab::Mesh {

/**
 * @brief Singleton mesh document manager
 */
class MeshDocumentManagerImpl : public MeshDocumentManager {
public:
    static std::shared_ptr<MeshDocumentManagerImpl> instance();

    [[nodiscard]] MeshDocumentPtr currentDocument() override;
    [[nodiscard]] MeshDocumentPtr newDocument() override;

    /// Typed accessor for internal use
    [[nodiscard]] std::shared_ptr<MeshDocumentImpl> currentDocumentImpl();

private:
    std::shared_ptr<MeshDocumentImpl> m_currentDocument;
};

class MeshDocumentManagerImplSingletonFactory : public IMeshDocumentManagerSingletonFactory {
public:
    tObjectSharedPtr instance() const override;
};

} // namespace OpenGeoLab::Mesh
