pragma ComponentBehavior: Bound

import QtQuick

import "../theme"
import "../components" as Components

Rectangle {
    id: viewportPanel

    required property AppTheme theme
    property string summaryText: ""
    property int recordedCommandCount: 0
    signal requestViewPage

    radius: theme.radiusLarge
    color: theme.surface
    border.width: 1
    border.color: theme.borderSubtle

    Rectangle {
        anchors.fill: parent
        anchors.margins: 14
        radius: 22
        color: viewportPanel.theme.viewportBase
        border.width: 1
        border.color: viewportPanel.theme.tint(viewportPanel.theme.accentA, viewportPanel.theme.darkMode ? 0.3 : 0.18)
        clip: true

        Canvas {
            anchors.fill: parent

            onPaint: {
                const ctx = getContext("2d");
                ctx.reset();
                ctx.fillStyle = viewportPanel.theme.viewportBase;
                ctx.fillRect(0, 0, width, height);

                ctx.strokeStyle = viewportPanel.theme.viewportGrid;
                ctx.globalAlpha = viewportPanel.theme.darkMode ? 0.34 : 0.56;
                ctx.lineWidth = 1;
                for (let x = 0; x < width; x += 34) {
                    ctx.beginPath();
                    ctx.moveTo(x, 0);
                    ctx.lineTo(x, height);
                    ctx.stroke();
                }
                for (let y = 0; y < height; y += 34) {
                    ctx.beginPath();
                    ctx.moveTo(0, y);
                    ctx.lineTo(width, y);
                    ctx.stroke();
                }

                const gradient = ctx.createLinearGradient(0, 0, width, height);
                gradient.addColorStop(0, viewportPanel.theme.tint(viewportPanel.theme.accentA, viewportPanel.theme.darkMode ? 0.18 : 0.08));
                gradient.addColorStop(0.5, viewportPanel.theme.tint(viewportPanel.theme.accentB, viewportPanel.theme.darkMode ? 0.12 : 0.06));
                gradient.addColorStop(1, viewportPanel.theme.tint(viewportPanel.theme.accentD, viewportPanel.theme.darkMode ? 0.14 : 0.05));
                ctx.fillStyle = gradient;
                ctx.fillRect(0, 0, width, height);

                ctx.globalAlpha = 1;
                ctx.fillStyle = viewportPanel.theme.tint(viewportPanel.theme.accentA, viewportPanel.theme.darkMode ? 0.2 : 0.12);
                ctx.beginPath();
                ctx.arc(width * 0.74, height * 0.3, Math.min(width, height) * 0.18, 0, Math.PI * 2);
                ctx.fill();

                ctx.fillStyle = viewportPanel.theme.tint(viewportPanel.theme.accentC, viewportPanel.theme.darkMode ? 0.14 : 0.09);
                ctx.beginPath();
                ctx.arc(width * 0.22, height * 0.72, Math.min(width, height) * 0.16, 0, Math.PI * 2);
                ctx.fill();
            }
        }

        Rectangle {
            id: orbitAccent

            width: 220
            height: 220
            radius: 38
            color: viewportPanel.theme.tint(viewportPanel.theme.accentA, viewportPanel.theme.darkMode ? 0.08 : 0.06)
            border.width: 1
            border.color: viewportPanel.theme.tint(viewportPanel.theme.accentA, 0.22)
            x: parent.width * 0.62
            y: parent.height * 0.16
            rotation: 12
            opacity: 0.8

            SequentialAnimation {
                loops: Animation.Infinite
                running: true

                NumberAnimation {
                    target: orbitAccent
                    property: "rotation"
                    from: 12
                    to: 18
                    duration: 2600
                    easing.type: Easing.InOutSine
                }

                NumberAnimation {
                    target: orbitAccent
                    property: "rotation"
                    from: 18
                    to: 12
                    duration: 2600
                    easing.type: Easing.InOutSine
                }
            }
        }

        Rectangle {
            id: floatingAccent

            width: 164
            height: 164
            radius: 24
            color: viewportPanel.theme.tint(viewportPanel.theme.accentD, viewportPanel.theme.darkMode ? 0.07 : 0.05)
            border.width: 1
            border.color: viewportPanel.theme.tint(viewportPanel.theme.accentD, 0.18)
            x: parent.width * 0.14
            y: parent.height * 0.52
            rotation: -16
            opacity: 0.84

            SequentialAnimation {
                loops: Animation.Infinite
                running: true

                NumberAnimation {
                    target: floatingAccent
                    property: "y"
                    from: viewportPanel.height * 0.52
                    to: viewportPanel.height * 0.49
                    duration: 2200
                    easing.type: Easing.InOutSine
                }

                NumberAnimation {
                    target: floatingAccent
                    property: "y"
                    from: viewportPanel.height * 0.49
                    to: viewportPanel.height * 0.52
                    duration: 2200
                    easing.type: Easing.InOutSine
                }
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.margins: 18
            radius: 18
            color: viewportPanel.theme.tint(viewportPanel.theme.surface, viewportPanel.theme.darkMode ? 0.78 : 0.9)
            border.width: 1
            border.color: viewportPanel.theme.tint(viewportPanel.theme.borderSubtle, 0.84)
            implicitWidth: overlayColumn.implicitWidth + 28
            implicitHeight: overlayColumn.implicitHeight + 22

            Column {
                id: overlayColumn

                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: 14
                spacing: 10

                Text {
                    text: "Render Workspace"
                    color: viewportPanel.theme.textPrimary
                    font.pixelSize: 18
                    font.bold: true
                    font.family: viewportPanel.theme.titleFontFamily
                }

                Text {
                    width: 320
                    wrapMode: Text.WordWrap
                    text: viewportPanel.summaryText
                    color: viewportPanel.theme.textSecondary
                    font.pixelSize: 13
                    font.family: viewportPanel.theme.bodyFontFamily
                }

                Row {
                    spacing: 8

                    Components.StatChip {
                        theme: viewportPanel.theme
                        text: "Selection"
                        tintColor: viewportPanel.theme.accentA
                    }

                    Components.StatChip {
                        theme: viewportPanel.theme
                        text: "Recorded " + viewportPanel.recordedCommandCount
                        tintColor: viewportPanel.theme.accentB
                    }
                }
            }
        }

        Column {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 18
            spacing: 10

            Components.RibbonTile {
                width: 72
                height: 70
                theme: viewportPanel.theme
                title: "View"
                iconKind: "eye"
                accentOne: viewportPanel.theme.accentA
                accentTwo: viewportPanel.theme.accentC
                onClicked: viewportPanel.requestViewPage()
            }
        }

        Text {
            anchors.centerIn: parent
            text: "OpenGL Viewport Placeholder"
            color: viewportPanel.theme.tint(viewportPanel.theme.textPrimary, 0.34)
            font.pixelSize: 30
            font.bold: true
            font.family: viewportPanel.theme.titleFontFamily
        }
    }
}
