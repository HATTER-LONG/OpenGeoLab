#include <opengeolab/app/progress_tracker.hpp>

#include <QMetaObject>

#include <algorithm>

namespace OpenGeoLab::App {

ProgressTracker::ProgressTracker(QObject* parent)
    : QObject(parent), prune_timer_(new QTimer(this)) {
    prune_timer_->setInterval(10'000);
    connect(prune_timer_, &QTimer::timeout, this, &ProgressTracker::pruneCompletedTasks);
    prune_timer_->start();
}

void ProgressTracker::beginTask(const QString& task_id, const QString& description) {
    {
        const std::lock_guard lock(mutex_);
        tasks_[task_id] =
            TaskState{description, {}, 0.0, std::chrono::steady_clock::now(), false, true};
    }

    emitProgressChanged();
}

void ProgressTracker::updateProgress(const QString& task_id,
                                     double progress,
                                     const QString& message) {
    {
        const std::lock_guard lock(mutex_);
        const auto iterator = tasks_.find(task_id);
        if(iterator == tasks_.end()) {
            return;
        }

        iterator->second.progress = progress;
        iterator->second.message = message;
        iterator->second.last_update = std::chrono::steady_clock::now();
    }

    emitProgressChanged();
}

void ProgressTracker::completeTask(const QString& task_id, bool success) {
    {
        const std::lock_guard lock(mutex_);
        const auto iterator = tasks_.find(task_id);
        if(iterator == tasks_.end()) {
            return;
        }

        iterator->second.completed = true;
        iterator->second.success = success;
        if(success) {
            iterator->second.progress = 1.0;
        }
        iterator->second.last_update = std::chrono::steady_clock::now();
    }

    emitProgressChanged();
}

double ProgressTracker::currentProgress() const {
    const std::lock_guard lock(mutex_);

    const TaskState* latest_task = nullptr;
    for(const auto& [task_id, task_state] : tasks_) {
        static_cast<void>(task_id);
        if(task_state.completed) {
            continue;
        }
        if(latest_task == nullptr || latest_task->last_update < task_state.last_update) {
            latest_task = &task_state;
        }
    }

    if(latest_task == nullptr) {
        return -1.0;
    }

    return latest_task->progress;
}

QString ProgressTracker::statusText() const {
    const std::lock_guard lock(mutex_);

    const TaskState* latest_task = nullptr;
    for(const auto& [task_id, task_state] : tasks_) {
        static_cast<void>(task_id);
        if(task_state.completed) {
            continue;
        }
        if(latest_task == nullptr || latest_task->last_update < task_state.last_update) {
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
    const std::lock_guard lock(mutex_);
    return std::any_of(tasks_.cbegin(), tasks_.cend(),
                       [](const auto& entry) { return !entry.second.completed; });
}

void ProgressTracker::emitProgressChanged() {
    QMetaObject::invokeMethod(this, &ProgressTracker::progressChanged, Qt::QueuedConnection);
}

void ProgressTracker::pruneCompletedTasks() {
    bool erased = false;
    {
        const std::lock_guard lock(mutex_);
        const auto now = std::chrono::steady_clock::now();
        const auto previous_size = tasks_.size();
        std::erase_if(tasks_, [now](const auto& entry) {
            return entry.second.completed &&
                   (now - entry.second.last_update) > std::chrono::seconds{10};
        });
        erased = tasks_.size() != previous_size;
    }

    if(erased) {
        emitProgressChanged();
    }
}

} // namespace OpenGeoLab::App
