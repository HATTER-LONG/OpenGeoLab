/**
 * @file log_entry_filter_model.hpp
 * @brief Proxy model for filtering log entries by level
 */

#pragma once

#include <app/log_entry_model.hpp>

#include <QSortFilterProxyModel>

namespace OpenGeoLab::App {

class LogEntryFilterModel final : public QSortFilterProxyModel {
    Q_OBJECT
    Q_PROPERTY(int minLevel READ minLevel WRITE setMinLevel NOTIFY minLevelChanged)
public:
    explicit LogEntryFilterModel(QObject* parent = nullptr);

    [[nodiscard]] int minLevel() const;
    void setMinLevel(int level);

    [[nodiscard]] bool levelEnabled(int level) const;
    void setLevelEnabled(int level, bool enabled);

signals:
    void minLevelChanged();
    void levelFilterChanged();

protected:
    [[nodiscard]] bool filterAcceptsRow(int sourceRow,
                                        const QModelIndex& sourceParent) const override;

private:
    int m_minLevel{0};
    quint32 m_enabledMask{0x3F};
};

} // namespace OpenGeoLab::App
