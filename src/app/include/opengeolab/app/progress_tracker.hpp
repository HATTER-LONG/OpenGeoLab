/**
 * @file progress_tracker.hpp
 * @brief Thread-safe task progress tracker exposed as QML-bindable properties.
 */

#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

#include <chrono>
#include <mutex>
#include <unordered_map>

namespace OpenGeoLab::App {

/**
 * @brief Tracks progress of concurrent async tasks, exposing aggregated state to QML.
 *
 * All public methods are thread-safe. Property change notifications are delivered
 * on the main thread via QueuedConnection.
 */
class ProgressTracker : public QObject {
    Q_OBJECT

    /// Aggregated progress of the most recently updated active task.
    /// -1 = no active tasks, 0 = indeterminate, (0,1] = determined progress.
    Q_PROPERTY(double currentProgress READ currentProgress NOTIFY progressChanged)

    /// Human-readable status text from the most recently updated active task.
    Q_PROPERTY(QString statusText READ statusText NOTIFY progressChanged)

    /// True when at least one task is active (not completed).
    Q_PROPERTY(bool hasActiveTasks READ hasActiveTasks NOTIFY progressChanged)

public:
    explicit ProgressTracker(QObject* parent = nullptr);

    /// @brief Create a new tracked task. Thread-safe.
    /// @param task_id Unique task identifier (typically RequestService's requestId).
    /// @param description Human-readable task description.
    void beginTask(const QString& task_id, const QString& description);

    /// @brief Update progress of a tracked task. Thread-safe.
    /// @param task_id Task identifier.
    /// @param progress [0, 1] for determined progress, 0 for indeterminate.
    /// @param message Optional status message.
    void updateProgress(const QString& task_id, double progress, const QString& message = {});

    /// @brief Mark a task as completed. Thread-safe.
    /// @param task_id Task identifier.
    /// @param success true for successful completion, false for failure.
    void completeTask(const QString& task_id, bool success = true);

    [[nodiscard]] double currentProgress() const;
    [[nodiscard]] QString statusText() const;
    [[nodiscard]] bool hasActiveTasks() const;

signals:
    void progressChanged();

private:
    struct TaskState {
        QString description;
        QString message;
        double progress = 0.0;
        std::chrono::steady_clock::time_point lastUpdate;
        bool completed = false;
        bool success = true;
    };

    void emitProgressChanged();
    void pruneCompletedTasks();

    mutable std::mutex m_mutex;
    std::unordered_map<QString, TaskState> m_tasks;
    QTimer* m_pruneTimer = nullptr;
};

} // namespace OpenGeoLab::App
