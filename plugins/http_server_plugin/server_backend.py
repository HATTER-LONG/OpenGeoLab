"""QObject bridge between ServerCore and QML management panel.

Exposes properties, signals and slots that the ServerWindow.qml UI binds to.
All property changes emit signals for QML reactivity.
"""
from __future__ import annotations

from PySide6.QtCore import Property, QObject, QTimer, Signal, Slot

from http_server_plugin.server_core import ServerCore


class ServerBackend(QObject):
    """Bridge between :class:`ServerCore` and the QML management panel."""

    hostChanged = Signal()
    portChanged = Signal()
    runningChanged = Signal()
    requestLogChanged = Signal()
    errorOccurred = Signal(str)
    requestReceived = Signal(dict)

    def __init__(self, parent: QObject | None = None) -> None:
        super().__init__(parent)
        self._core = ServerCore()
        self._host = "127.0.0.1"
        self._port = 8080
        self._running = False
        self._request_log: list[dict[str, object]] = []

        self._poll_timer = QTimer(self)
        self._poll_timer.setInterval(500)
        self._poll_timer.timeout.connect(self.refreshLog)

    @Property(str, notify=hostChanged)
    def host(self) -> str:
        return self._host

    @host.setter
    def host(self, value: str) -> None:
        if self._host == value:
            return
        self._host = value
        self.hostChanged.emit()

    @Property(int, notify=portChanged)
    def port(self) -> int:
        return self._port

    @port.setter
    def port(self, value: int) -> None:
        if self._port == value:
            return
        self._port = value
        self.portChanged.emit()

    @Property(bool, notify=runningChanged)
    def running(self) -> bool:
        return self._running

    @Property("QVariantList", notify=requestLogChanged)
    def requestLog(self) -> list[dict[str, object]]:
        return self._request_log

    @Slot()
    def start(self) -> None:
        """Start the HTTP server on the configured host and port."""
        try:
            self._core.start(self._host, self._port)
        except Exception as exc:
            self._set_running(False)
            self.errorOccurred.emit(str(exc))
            return

        self._sync_port_from_core()
        self._set_running(self._core.is_running())
        if self._running:
            self._poll_timer.start()

    @Slot()
    def stop(self) -> None:
        """Stop the HTTP server."""
        self._poll_timer.stop()
        self._core.stop()
        self._set_running(self._core.is_running())

    @Slot()
    def refreshLog(self) -> None:
        """Refresh the request log from the core."""
        new_log = self._core.get_request_log()
        if new_log == self._request_log:
            return
        self._request_log = new_log
        self.requestLogChanged.emit()

    def _set_running(self, running: bool) -> None:
        if self._running == running:
            return
        self._running = running
        self.runningChanged.emit()

    def _sync_port_from_core(self) -> None:
        core_port = self._core.port
        if core_port <= 0 or core_port == self._port:
            return
        self._port = core_port
        self.portChanged.emit()
