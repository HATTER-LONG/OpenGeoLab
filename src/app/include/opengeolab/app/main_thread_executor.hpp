/**
 * @file main_thread_executor.hpp
 * @brief Cross-thread main-thread task executor with GIL deadlock prevention.
 */
#pragma once

#include <QObject>

#include <functional>

namespace OpenGeoLab::App {

/**
 * @brief Executes callables on the main (GUI) thread from any thread.
 *
 * Use for operations that require main-thread affinity, such as PySide6 window
 * creation or OpenGL context access.
 */
class MainThreadExecutor : public QObject {
    Q_OBJECT

public:
    explicit MainThreadExecutor(QObject* parent = nullptr);

    /**
     * @brief Execute a callable on the main thread asynchronously.
     *
     * If already on the main thread, executes immediately.
     * Otherwise queues via QueuedConnection.
     * @param task Callable to execute.
     */
    void execute(std::function<void()> task);

    /**
     * @brief Execute a callable on the main thread and block until done.
     *
     * If already on the main thread, executes immediately.
     * Otherwise uses BlockingQueuedConnection.
     *
     * @warning Caller must NOT hold the Python GIL when calling this method.
     * If the task needs to acquire the GIL, a deadlock will occur:
     *   - Calling thread: holds GIL -> waits for main thread
     *   - Main thread: tries to acquire GIL -> blocked by calling thread
     * Use the non-blocking execute() if unsure.
     * @param task Callable to execute.
     */
    void executeBlocking(std::function<void()> task);
};

} // namespace OpenGeoLab::App
