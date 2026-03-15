/**
 * @file OperationLogService.hpp
 * @brief Thread-safe QObject service exposing application logs to QML.
 */

#pragma once

#include "OperationLogModel.hpp"

#include <QAbstractItemModel>
#include <QObject>

namespace OGL::App {

class OperationLogService final : public QObject {
    Q_OBJECT

    Q_PROPERTY(QAbstractItemModel* model READ model CONSTANT)
    Q_PROPERTY(bool hasNewErrors READ hasNewErrors NOTIFY hasNewErrorsChanged)
    Q_PROPERTY(bool hasNewLogs READ hasNewLogs NOTIFY hasNewLogsChanged)
    Q_PROPERTY(int minLevel READ minLevel WRITE setMinLevel NOTIFY minLevelChanged)
    Q_PROPERTY(int enabledLevelMask READ enabledLevelMask NOTIFY levelFilterChanged)

public:
    explicit OperationLogService(QObject* parent = nullptr);

    [[nodiscard]] QAbstractItemModel* model();
    [[nodiscard]] bool hasNewErrors() const;
    [[nodiscard]] bool hasNewLogs() const;
    [[nodiscard]] int minLevel() const;
    [[nodiscard]] int enabledLevelMask() const;

    Q_INVOKABLE void setMinLevel(int level);
    Q_INVOKABLE bool levelEnabled(int level) const;
    Q_INVOKABLE void setLevelEnabled(int level, bool enabled);
    void addEntry(OperationLogEntry entry);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void markAllSeen();

signals:
    void hasNewErrorsChanged();
    void hasNewLogsChanged();
    void minLevelChanged();
    void levelFilterChanged();

private:
    void addEntryOnUiThread(OperationLogEntry entry);

    OperationLogModel m_model;
    OperationLogFilterModel m_filterModel;
    bool m_hasNewErrors = false;
    bool m_hasNewLogs = false;
};

} // namespace OGL::App
