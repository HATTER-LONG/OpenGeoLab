#include <app/log_entry_filter_model.hpp>

namespace OpenGeoLab::App {

LogEntryFilterModel::LogEntryFilterModel(QObject* parent) : QSortFilterProxyModel(parent) {
    setDynamicSortFilter(true);
}

int LogEntryFilterModel::minLevel() const { return m_minLevel; }

void LogEntryFilterModel::setMinLevel(int level) {
    if(m_minLevel == level) {
        return;
    }
    m_minLevel = level;
    invalidateFilter();
    emit minLevelChanged();
}

bool LogEntryFilterModel::levelEnabled(int level) const {
    if(level < 0 || level > 31) {
        return false;
    }
    return ((m_enabledMask >> static_cast<quint32>(level)) & 0x1u) != 0u;
}

void LogEntryFilterModel::setLevelEnabled(int level, bool enabled) {
    if(level < 0 || level > 31) {
        return;
    }
    const quint32 bit = 1u << static_cast<quint32>(level);
    const quint32 mask = enabled ? (m_enabledMask | bit) : (m_enabledMask & ~bit);
    if(mask == m_enabledMask) {
        return;
    }
    m_enabledMask = mask;
    invalidateFilter();
    emit levelFilterChanged();
}

bool LogEntryFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    if(!sourceModel()) {
        return true;
    }

    const auto index = sourceModel()->index(sourceRow, 0, sourceParent);
    const auto level = index.data(LogEntryModel::LevelRole).toInt();
    if(level < m_minLevel) {
        return false;
    }

    if(!levelEnabled(level)) {
        return false;
    }

    return true;
}

} // namespace OpenGeoLab::App
