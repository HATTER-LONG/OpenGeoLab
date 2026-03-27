pragma ComponentBehavior: Bound

import QtQuick
import "../theme"

/** @brief Placeholder 3D viewport with a subtle grid background. */
Item {
    id: root

    required property AppTheme theme

    // ── Background gradient ────────────────────────────────────────────
    Rectangle {
        anchors.fill: parent
        radius: root.theme.radiusMedium
        color: root.theme.viewportBase

        // Grid pattern drawn on a Canvas
        Canvas {
            id: gridCanvas
            anchors.fill: parent
            onPaint: {
                var ctx = getContext("2d");
                ctx.clearRect(0, 0, width, height);
                ctx.strokeStyle = root.theme.viewportGrid;
                ctx.lineWidth = 0.5;
                var step = 40;
                for (var x = 0; x < width; x += step) {
                    ctx.beginPath();
                    ctx.moveTo(x, 0);
                    ctx.lineTo(x, height);
                    ctx.stroke();
                }
                for (var y = 0; y < height; y += step) {
                    ctx.beginPath();
                    ctx.moveTo(0, y);
                    ctx.lineTo(width, y);
                    ctx.stroke();
                }
            }
        }

        // Repaint when viewport changes size or theme toggles
        Connections {
            target: root.theme
            function onDarkModeChanged() {
                gridCanvas.requestPaint();
            }
        }

        onWidthChanged: gridCanvas.requestPaint()
        onHeightChanged: gridCanvas.requestPaint()

        // Centered label
        Text {
            anchors.centerIn: parent
            text: qsTr("3D Viewport")
            font.pixelSize: 28
            font.weight: Font.Bold
            color: root.theme.tint(root.theme.textTertiary, 0.35)
        }
    }
}
