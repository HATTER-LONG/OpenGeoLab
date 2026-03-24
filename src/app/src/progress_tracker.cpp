#include <opengeolab/app/progress_tracker.hpp>

#include <QMetaObject>

#include <algorithm>

namespace OpenGeoLab::App {

ProgressTracker::ProgressTracker(QObject* parent)
    : QObject(parent), m_pruneTimer(new QTimer(this)) {
    m_pruneTimer->setInterval(10'000);
    connect(m_pruneTimer, &QTimer::timeout, this, &ProgressTracker::pruneCompletedTasks);
    m_pruneTimer->start();
}

void ProgressTracker::beginTask(const QString& task_id, const QString& description) {
    {
        const std::lock_guard lock(m_mutex);
        m_tasks[task_id] =
            TaskState{description, {}, 0.0, std::chrono::steady_clock::now(), false, true};
    }

    // Emit taskStarted before progressChanged so QML resets bar state
    // before bindings see the new progress value.
    QMetaObject::invokeMethod(
        this, [this, task_id]() { emit taskStarted(task_id); }, Qt::QueuedConnection);

    emitProgressChanged();
}

void ProgressTracker::updateProgress(const QString& task_id,
                                     double progress,
                                     const QString& message) {
    {
        const std::lock_guard lock(m_mutex);
        const auto iterator = m_tasks.find(task_id);
        if(iterator == m_tasks.end()) {
            return;
        }

        iterator->second.progress = progress;
        iterator->second.message = message;
        iterator->second.lastUpdate = std::chrono::steady_clock::now();
    }

    emitProgressChanged();
}

void ProgressTracker::completeTask(const QString& task_id, bool success) {
    {
        const std::lock_guard lock(m_mutex);
        const auto iterator = m_tasks.find(task_id);
        if(iterator == m_tasks.end()) {
            return;
        }

        iterator->second.completed = true;
        iterator->second.success = success;
        if(success) {
            iterator->second.progress = 1.0;
        }
        iterator->second.lastUpdate = std::chrono::steady_clock::now();
    }

    // Emit taskCompleted BEFORE progressChanged so QML sets completionState
    // before bindings see hasActiveTasks=false. This avoids the progress bar
    // briefly jumping to 0 then animating back to 100%.
    QMetaObject::invokeMethod(
        this, [this, task_id, success]() { emit taskCompleted(task_id, success); },
        Qt::QueuedConnection);

    emitProgressChanged();
}

double ProgressTracker::currentProgress() const {
    const std::lock_guard lock(m_mutex);

    const TaskState* latest_task = nullptr;
    for(const auto& [task_id, task_state] : m_tasks) {
        static_cast<void>(task_id);
        if(task_state.completed) {
            continue;
        }
        if(latest_task == nullptr || latest_task->lastUpdate < task_state.lastUpdate) {
            latest_task = &task_state;
        }
    }

    if(latest_task == nullptr) {
        return -1.0;
    }

    return latest_task->progress;
}

QString ProgressTracker::statusText() const {
    const std::lock_guard lock(m_mutex);

    const TaskState* latest_task = nullptr;
    for(const auto& [task_id, task_state] : m_tasks) {
        static_cast<void>(task_id);
        if(task_state.completed) {
            continue;
        }
        if(latest_task == nullptr || latest_task->lastUpdate < task_state.lastUpdate) {
            latest_task = &task_state;
        }
    }

    if(latest_task == nullptr) {
        return {};
    }

    if(latest_task->message.isEmpty()) {
        return latest_task->description;
    }

    return QStringLiteral("%1: %2").arg(latest_task->description, latest_task->message);
}

bool ProgressTracker::hasActiveTasks() const {
    const std::lock_guard lock(m_mutex);
    return std::any_of(m_tasks.cbegin(), m_tasks.cend(),
                       [](const auto& entry) { return !entry.second.completed; });
}

QString ProgressTracker::currentMessage() const {
    const std::lock_guard lock(m_mutex);

    const TaskState* latest_task = nullptr;
    for(const auto& [task_id, task_state] : m_tasks) {
        static_cast<void>(task_id);
        if(task_state.completed) {
            continue;
        }
        if(latest_task == nullptr || latest_task->lastUpdate < task_state.lastUpdate) {
            latest_task = &task_state;
        }
    }

    if(latest_task == nullptr) {
        return {};
    }

    return latest_task->message;
}

void ProgressTracker::emitProgressChanged() {
    QMetaObject::invokeMethod(this, &ProgressTracker::progressChanged, Qt::QueuedConnection);
}

void ProgressTracker::pruneCompletedTasks() {
    bool erased = false;
    {
        const std::lock_guard lock(m_mutex);
        const auto now = std::chrono::steady_clock::now();
        const auto previous_size = m_tasks.size();
        std::erase_if(m_tasks, [now](const auto& entry) {
            return entry.second.completed &&
                   (now - entry.second.lastUpdate) > std::chrono::seconds{10};
        });
        erased = m_tasks.size() != previous_size;
    }

    if(erased) {
        emitProgressChanged();
    }
}

} // namespace OpenGeoLab::App
