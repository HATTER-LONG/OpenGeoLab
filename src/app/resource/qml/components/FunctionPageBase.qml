import QtQuick
import QtQuick.Layouts
import OpenGeoLab.Services 1.0
import ".."
import "../theme"

Item {
    id: root

    property AppTheme theme: MainPages.theme

    property string pageTitle: qsTr("Function")
    property string pageIcon: ""
    property string actionId: ""
    property bool pageVisible: false
    property int maxContentHeight: 420
    default property alias content: contentColumn.data

    visible: pageVisible
    focus: pageVisible
    z: 1000
    width: 320
    height: panelColumn.implicitHeight

    function clamp(value, minimum, maximum) {
        return Math.max(minimum, Math.min(maximum, value));
    }

    function open(payload) {
        root.x = 292;
        root.y = 0;
        root.pageVisible = true;
        root.forceActiveFocus();
        if (payload !== undefined && payload !== null) {
            root.parsePayload(payload);
        }
    }

    function close() {
        root.pageVisible = false;
        if (MainPages.currentOpenPage === root.actionId) {
            MainPages.currentOpenPage = "";
        }
    }

    function parsePayload(payload) {
    }

    function getParameters() {
        return {};
    }

    function execute() {
        RequestService.submitAsync(JSON.stringify(root.getParameters()));
        root.close();
    }

    Keys.onEscapePressed: root.close()

    Rectangle {
        id: shadow

        anchors.fill: panel
        anchors.margins: -2
        radius: panel.radius + 2
        color: root.theme.tint(root.theme.shell, root.theme.darkMode ? 0.18 : 0.08)
    }

    Rectangle {
        id: panel

        anchors.fill: parent
        radius: root.theme.radiusMedium
        color: root.theme.surface
        border.width: 1
        border.color: root.theme.borderSubtle
        clip: true

        Column {
            id: panelColumn

            anchors.fill: parent
            anchors.margins: 1
            spacing: 0

            Rectangle {
                id: titleBar

                width: parent.width
                height: 36
                radius: root.theme.radiusMedium - 1
                color: root.theme.surfaceMuted

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: root.theme.radiusMedium
                    color: parent.color
                }

                MouseArea {
                    id: dragArea

                    property real pressOffsetX: 0
                    property real pressOffsetY: 0

                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    cursorShape: Qt.SizeAllCursor
                    preventStealing: true

                    onPressed: function(mouse) {
                        dragArea.pressOffsetX = mouse.x;
                        dragArea.pressOffsetY = mouse.y;
                    }

                    onPositionChanged: function(mouse) {
                        if (!dragArea.pressed || !root.parent) {
                            return;
                        }
                        const nextX = root.x + mouse.x - dragArea.pressOffsetX;
                        const nextY = root.y + mouse.y - dragArea.pressOffsetY;
                        const minX = 292;
                        const maxX = Math.max(minX, root.parent.width - root.width);
                        const minY = 0;
                        const maxY = Math.max(minY, root.parent.height - root.height);
                        root.x = root.clamp(nextX, minX, maxX);
                        root.y = root.clamp(nextY, minY, maxY);
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 6
                    spacing: 8

                    AppIcon {
                        theme: root.theme
                        iconKind: root.pageIcon
                        width: 18
                        height: 18
                        visible: root.pageIcon.length > 0
                    }

                    Text {
                        Layout.fillWidth: true
                        text: root.pageTitle
                        color: root.theme.textPrimary
                        font.pixelSize: 13
                        font.bold: true
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }

                    Rectangle {
                        id: closeButton

                        Layout.preferredWidth: 24
                        Layout.preferredHeight: 24
                        radius: root.theme.radiusSmall
                        color: closeMouseArea.pressed
                            ? root.theme.tint(root.theme.danger, root.theme.darkMode ? 0.38 : 0.22)
                            : (closeMouseArea.containsMouse
                                   ? root.theme.tint(root.theme.danger, root.theme.darkMode ? 0.28 : 0.14)
                                   : "transparent")
                        border.width: closeMouseArea.containsMouse ? 1 : 0
                        border.color: root.theme.tint(root.theme.danger, root.theme.darkMode ? 0.56 : 0.36)
                        scale: closeMouseArea.pressed ? 0.92 : (closeMouseArea.containsMouse ? 1.06 : 1.0)

                        Behavior on color {
                            ColorAnimation {
                                duration: 140
                            }
                        }

                        Behavior on scale {
                            NumberAnimation {
                                duration: 120
                                easing.type: Easing.OutCubic
                            }
                        }

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("✕")
                            color: closeMouseArea.containsMouse
                                ? root.theme.danger
                                : root.theme.textSecondary
                            font.pixelSize: 13
                            font.bold: true

                            Behavior on color {
                                ColorAnimation {
                                    duration: 140
                                }
                            }
                        }

                        MouseArea {
                            id: closeMouseArea

                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.close()
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: root.theme.borderSubtle
            }

            Flickable {
                id: contentArea

                width: parent.width
                height: Math.min(contentColumn.implicitHeight + 24, root.maxContentHeight)
                clip: true
                contentWidth: width
                contentHeight: contentColumn.implicitHeight + 24
                boundsBehavior: Flickable.StopAtBounds
                interactive: contentHeight > height

                Column {
                    id: contentColumn

                    x: 12
                    y: 12
                    width: contentArea.width - 24
                    spacing: 12
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: root.theme.borderSubtle
            }

            Item {
                width: parent.width
                height: 48

                RowLayout {
                    anchors.centerIn: parent
                    spacing: 12

                    Rectangle {
                        id: executeButton

                        Layout.preferredWidth: 112
                        Layout.preferredHeight: 32
                        radius: root.theme.radiusSmall
                        color: executeMouseArea.pressed
                            ? Qt.darker(root.theme.accentA, 1.15)
                            : (executeMouseArea.containsMouse
                                   ? Qt.lighter(root.theme.accentA, 1.12)
                                   : root.theme.accentA)
                        border.width: 1
                        border.color: executeMouseArea.containsMouse
                            ? Qt.lighter(root.theme.accentA, 1.3)
                            : Qt.darker(root.theme.accentA, 1.1)
                        scale: executeMouseArea.pressed ? 0.97 : (executeMouseArea.containsMouse ? 1.02 : 1.0)

                        Behavior on color {
                            ColorAnimation {
                                duration: 140
                            }
                        }

                        Behavior on scale {
                            NumberAnimation {
                                duration: 120
                                easing.type: Easing.OutCubic
                            }
                        }

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Execute")
                            color: "#ffffff"
                            font.pixelSize: 13
                            font.bold: true
                        }

                        MouseArea {
                            id: executeMouseArea

                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.execute()
                        }
                    }

                    Rectangle {
                        id: cancelButton

                        Layout.preferredWidth: 96
                        Layout.preferredHeight: 32
                        radius: root.theme.radiusSmall
                        color: cancelMouseArea.pressed
                            ? root.theme.surfaceStrong
                            : (cancelMouseArea.containsMouse
                                   ? root.theme.tint(root.theme.surfaceMuted, root.theme.darkMode ? 0.96 : 0.9)
                                   : root.theme.surfaceMuted)
                        border.width: 1
                        border.color: cancelMouseArea.containsMouse
                            ? root.theme.tint(root.theme.textPrimary, root.theme.darkMode ? 0.52 : 0.3)
                            : root.theme.borderSubtle
                        scale: cancelMouseArea.pressed ? 0.98 : (cancelMouseArea.containsMouse ? 1.01 : 1.0)

                        Behavior on color {
                            ColorAnimation {
                                duration: 140
                            }
                        }

                        Behavior on scale {
                            NumberAnimation {
                                duration: 120
                                easing.type: Easing.OutCubic
                            }
                        }

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Cancel")
                            color: root.theme.textPrimary
                            font.pixelSize: 13
                            font.bold: true
                        }

                        MouseArea {
                            id: cancelMouseArea

                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.close()
                        }
                    }
                }
            }
        }
    }
}
