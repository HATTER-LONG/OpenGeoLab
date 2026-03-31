/**
 * @file log_filter_proxy_model.h
 * @brief QSortFilterProxyModel that filters log entries by severity level bitmask.
 *
 * Designed to wrap LogEventModel and provide filtered output to QML ListView,
 * eliminating the need for delegate-side visibility hacks that cause blank
 * spaces when ListView spacing is non-zero.
 */

#pragma once

#include <QSortFilterProxyModel>

namespace OpenGeoLab::App {

/**
 * @brief Proxy model filtering log entries by an enabled-level bitmask.
 *
 * Each bit in enabledLevelMask corresponds to a spdlog level (0–5).
 * When a bit is set, entries of that level pass through; otherwise they
 * are hidden from the view.
 *
 * Usage in QML:
 * @code
 * LogFilterProxyModel {
 *     id: filterProxy
 *     sourceModel: LogEventModel
 *     enabledLevelMask: root.enabledLevelMask
 * }
 * ListView { model: filterProxy }
 * @endcode
 */
class LogFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
    Q_PROPERTY(int enabledLevelMask READ enabledLevelMask WRITE setEnabledLevelMask NOTIFY
                   enabledLevelMaskChanged)

public:
    explicit LogFilterProxyModel(QObject* parent = nullptr);

    [[nodiscard]] int enabledLevelMask() const;
    void setEnabledLevelMask(int mask);

Q_SIGNALS:
    void enabledLevelMaskChanged();

protected:
    /**
     * @brief Accept rows whose level bit is set in the mask.
     * @param source_row Row index in the source model.
     * @param source_parent Parent index (unused for flat models).
     * @return True if the entry's level is enabled.
     */
    [[nodiscard]] bool filterAcceptsRow(int source_row,
                                        const QModelIndex& source_parent) const override;

private:
    int m_enabledLevelMask{0x3F}; ///< All six levels enabled by default.
};

} // namespace OpenGeoLab::App
