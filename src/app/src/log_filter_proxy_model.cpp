/**
 * @file log_filter_proxy_model.cpp
 * @brief Implementation of LogFilterProxyModel.
 */

#include "opengeolab/app/log_filter_proxy_model.h"

#include "opengeolab/app/log_event_model.h"

namespace OpenGeoLab::App {

LogFilterProxyModel::LogFilterProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}

int LogFilterProxyModel::enabledLevelMask() const { return m_enabledLevelMask; }

void LogFilterProxyModel::setEnabledLevelMask(int mask) {
    if(m_enabledLevelMask == mask) {
        return;
    }
    m_enabledLevelMask = mask;
    invalidateFilter();
    emit enabledLevelMaskChanged();
}

bool LogFilterProxyModel::filterAcceptsRow(int source_row,
                                           const QModelIndex& source_parent) const {
    const auto index = sourceModel()->index(source_row, 0, source_parent);
    const int level = sourceModel()->data(index, LogEventModel::LevelRole).toInt();
    return (m_enabledLevelMask & (1 << level)) != 0;
}

} // namespace OpenGeoLab::App
