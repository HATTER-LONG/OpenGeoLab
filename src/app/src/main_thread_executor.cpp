#include <opengeolab/app/main_thread_executor.hpp>

#include <QMetaObject>
#include <QThread>

#ifdef Py_PYTHON_H
#include <Python.h>
#endif

namespace OpenGeoLab::App {

MainThreadExecutor::MainThreadExecutor(QObject* parent) : QObject(parent) {}

void MainThreadExecutor::execute(std::function<void()> task) {
    if(QThread::currentThread() == thread()) {
        task();
        return;
    }

    QMetaObject::invokeMethod(this, [task = std::move(task)]() { task(); }, Qt::QueuedConnection);
}

void MainThreadExecutor::executeBlocking(std::function<void()> task) {
    if(QThread::currentThread() == thread()) {
        task();
        return;
    }

    // Debug builds assert that the caller does not hold the GIL,
    // preventing deadlock with BlockingQueuedConnection.
#ifdef Py_PYTHON_H
    Q_ASSERT(PyGILState_Check() == 0);
#endif

    QMetaObject::invokeMethod(
        this, [task = std::move(task)]() { task(); }, Qt::BlockingQueuedConnection);
}

} // namespace OpenGeoLab::App
