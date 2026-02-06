#pragma once
#include "util/signal.hpp"
#include <QObject>
#include <QtQml/qqml.h>

namespace OpenGeoLab::App {

class SelectManagerService final : public QObject {
    Q_OBJECT

    QML_ELEMENT
    QML_SINGLETON
public:
    explicit SelectManagerService(QObject* parent = nullptr);
    ~SelectManagerService() override;

    // =========================================================================
    // Selection Management
    // =========================================================================

    Q_INVOKABLE void activateSelectMode(uint32_t select_types);

    Q_INVOKABLE void deactivateSelectMode();

    Q_INVOKABLE bool isSelectEnabled() const;

    Q_INVOKABLE void clearSelection();

    Q_INVOKABLE void selectEntity(uint32_t entity_uid, const QString& entity_type);

    Q_INVOKABLE void removeEntity(uint32_t entity_uid, const QString& entity_type);

    Q_INVOKABLE bool isEntitySelected(uint32_t entity_uid, const QString& entity_type) const;

    Q_INVOKABLE QVector<QPair<uint32_t, QString>> currentSelections() const;

signals:
    void selectModeChanged(uint32_t select_types);
    void entitySelected(uint32_t entity_uid, const QString& entity_type);
    void entityRemoved(uint32_t entity_uid, const QString& entity_type);
    void selectModeActivated(bool enabled);
    void selectionCleared();

private:
    Util::ScopedConnection m_selectSettingsConn;
    Util::ScopedConnection m_selectionConn;
    Util::ScopedConnection m_selectionEnable;
};
} // namespace OpenGeoLab::App