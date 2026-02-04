#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QVariant>
#include <QtQml/qqml.h>

namespace OpenGeoLab::App {

class PickManager final : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QObject* viewport READ viewport WRITE setViewport NOTIFY viewportChanged)

    Q_PROPERTY(QString activeConsumerKey READ activeConsumerKey WRITE setActiveConsumer NOTIFY
                   activeConsumerKeyChanged)
    Q_PROPERTY(QString contextKey READ contextKey WRITE setContext NOTIFY contextKeyChanged)

    Q_PROPERTY(QString selectedType READ selectedType NOTIFY selectedTypeChanged)
    Q_PROPERTY(bool pickModeActive READ pickModeActive NOTIFY pickModeActiveChanged)

    Q_PROPERTY(QVariantList selectedEntities READ selectedEntities NOTIFY selectedEntitiesChanged)

    Q_PROPERTY(bool autoAddToSelection READ autoAddToSelection WRITE setAutoAddToSelection NOTIFY
                   autoAddToSelectionChanged)

    Q_PROPERTY(bool singlePickActive READ singlePickActive NOTIFY singlePickActiveChanged)

public:
    explicit PickManager(QObject* parent = nullptr);
    ~PickManager() override;

    QObject* viewport() const;
    Q_INVOKABLE void setViewport(QObject* vp);

    QString activeConsumerKey() const;
    Q_INVOKABLE void setActiveConsumer(const QString& key);

    QString contextKey() const;
    Q_INVOKABLE void setContext(const QString& key);

    Q_INVOKABLE void clearActiveConsumer();

    QString selectedType() const;
    bool pickModeActive() const;

    QVariantList selectedEntities() const;

    bool autoAddToSelection() const;
    void setAutoAddToSelection(bool enabled);

    bool singlePickActive() const;

    // Selection management
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void addSelection(const QString& entityType, int entityUid);
    Q_INVOKABLE void removeSelection(const QString& entityType, int entityUid);

    // Pick mode management
    Q_INVOKABLE void activatePickMode(const QString& entityType);
    Q_INVOKABLE void deactivatePickMode();

    // Single-pick mode (signal callback)
    Q_INVOKABLE void requestSinglePick(const QString& entityType, const QString& consumerKey = {});

signals:
    void viewportChanged();

    void activeConsumerKeyChanged();
    void contextKeyChanged();

    void selectedTypeChanged();
    void pickModeActiveChanged();

    void selectedEntitiesChanged();
    void autoAddToSelectionChanged();

    void singlePickActiveChanged();

    // Compatibility/bridge signals used by QML UI
    void pickModeChanged(const QString& contextKey, bool enabled, const QString& entityType);
    void selectionChanged(const QString& contextKey, const QVariant& entities);
    void entityPicked(const QString& contextKey, const QString& entityType, int entityUid);
    void pickCancelled(const QString& contextKey);

    // Single pick callbacks
    void singlePickStarted(const QString& contextKey, const QString& entityType);
    void singlePickFinished(const QString& contextKey, const QString& entityType, int entityUid);
    void singlePickCancelled(const QString& contextKey);

private slots:
    void onViewportEntityPicked(const QString& entityType, int entityUid);
    void onViewportPickCancelled();

private:
    QString effectiveContextKey() const;
    void ensureContext(const QString& key);
    void loadSelectionForContext(const QString& key);
    void saveSelectionForContext(const QString& key);

    void applyViewportState();
    void setPickModeInternal(bool enabled, const QString& entityType);
    void setSelectedEntitiesInternal(const QVariantList& entities);

private:
    QPointer<QObject> m_viewport;

    QString m_activeConsumerKey;
    QString m_contextKey;

    QString m_selectedType;
    bool m_pickModeActive{false};

    QVariantList m_selectedEntities;
    bool m_autoAddToSelection{true};

    // per-context selection store
    QHash<QString, QVariantList> m_selectionByContext;

    // single pick
    bool m_singlePickActive{false};
    bool m_restoreAutoAddAfterSinglePick{true};

    QMetaObject::Connection m_connPicked;
    QMetaObject::Connection m_connCancelled;
};

} // namespace OpenGeoLab::App
