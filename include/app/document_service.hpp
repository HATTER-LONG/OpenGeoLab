/**
 * @file document_service.hpp
 * @brief QML-exposed document service for geometry document access
 *
 * DocumentService provides QML access to the current geometry document's
 * entity information, including parts and their sub-entity counts.
 */

#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QtQml/qqml.h>

namespace OpenGeoLab::App {

/**
 * @brief Information about a Part entity for display in the UI
 */
struct PartInfo {
    Q_GADGET
    Q_PROPERTY(qint64 entityId MEMBER m_entityId)
    Q_PROPERTY(QString name MEMBER m_name)
    Q_PROPERTY(int vertexCount MEMBER m_vertexCount)
    Q_PROPERTY(int edgeCount MEMBER m_edgeCount)
    Q_PROPERTY(int wireCount MEMBER m_wireCount)
    Q_PROPERTY(int faceCount MEMBER m_faceCount)
    Q_PROPERTY(int shellCount MEMBER m_shellCount)
    Q_PROPERTY(int solidCount MEMBER m_solidCount)

public:
    qint64 m_entityId{0}; ///< Entity ID of the part
    QString m_name;       ///< Display name of the part
    int m_vertexCount{0}; ///< Number of vertex entities
    int m_edgeCount{0};   ///< Number of edge entities
    int m_wireCount{0};   ///< Number of wire entities
    int m_faceCount{0};   ///< Number of face entities
    int m_shellCount{0};  ///< Number of shell entities
    int m_solidCount{0};  ///< Number of solid entities
};

/**
 * @brief List model for Part entities in the document
 *
 * Provides part information to QML for display in the sidebar.
 * Automatically updates when the document changes.
 */
class PartListModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT

public:
    enum Roles {
        EntityIdRole = Qt::UserRole + 1,
        NameRole,
        VertexCountRole,
        EdgeCountRole,
        WireCountRole,
        FaceCountRole,
        ShellCountRole,
        SolidCountRole
    };

    explicit PartListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    /**
     * @brief Refresh the model from the current document
     */
    Q_INVOKABLE void refresh();

private:
    std::vector<PartInfo> m_parts;
};

/**
 * @brief QML singleton service for document information access
 *
 * Provides QML components with access to document metadata including
 * part list and entity counts. Subscribes to document changes and
 * emits signals when the document is modified.
 */
class DocumentService : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int partCount READ partCount NOTIFY documentChanged)
    Q_PROPERTY(int totalEntityCount READ totalEntityCount NOTIFY documentChanged)
    Q_PROPERTY(PartListModel* partListModel READ partListModel CONSTANT)

public:
    explicit DocumentService(QObject* parent = nullptr);
    ~DocumentService() override;

    /**
     * @brief Get the number of parts in the document
     * @return Part count
     */
    [[nodiscard]] int partCount() const;

    /**
     * @brief Get the total entity count in the document
     * @return Total entity count
     */
    [[nodiscard]] int totalEntityCount() const;

    /**
     * @brief Get the part list model for QML binding
     * @return Pointer to the part list model
     */
    [[nodiscard]] PartListModel* partListModel();

    /**
     * @brief Force refresh of document information
     */
    Q_INVOKABLE void refresh();

signals:
    /// Emitted when the document content changes
    void documentChanged();

private slots:
    void onDocumentChanged();

private:
    void subscribeToDocument();
    void updateCounts();

private:
    PartListModel* m_partListModel{nullptr};
    int m_partCount{0};
    int m_totalEntityCount{0};
};

} // namespace OpenGeoLab::App
