import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import theme

/**
 * Dynamic parameter form rendered from ParamListModel.
 * Uses a Repeater + Loader to pick type-appropriate delegates.
 */
Item {
    id: root

    required property var model
    required property bool isExecuting
    required property string executionState
    required property real progress
    required property string progressMessage

    signal executeClicked()
    signal clearClicked()
    signal statusDismissed()
    signal valueChanged(string name, var value)

    ColumnLayout {
        anchors.fill: parent
        spacing: PluginTheme.gapTight

        Label {
            text: qsTr("Parameters")
            color: PluginTheme.textSecondary
            font.pixelSize: 12
            font.weight: Font.DemiBold
            Layout.leftMargin: PluginTheme.gapTight
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: PluginTheme.surfaceMuted
            radius: PluginTheme.radiusSmall
            clip: true

            ScrollView {
                anchors.fill: parent
                anchors.margins: PluginTheme.gapTight
                contentWidth: availableWidth

                Column {
                    width: parent.width
                    spacing: PluginTheme.gapTight

                    Repeater {
                        id: paramRepeater
                        model: root.model

                        delegate: RowLayout {
                            id: paramRow
                            width: parent.width
                            spacing: PluginTheme.gapTight
                            opacity: model.enabled ? 1.0 : 0.4

                            Behavior on opacity {
                                NumberAnimation { duration: PluginTheme.animFast }
                            }

                            // Optional param checkbox
                            CheckBox {
                                visible: !model.required
                                checked: model.enabled
                                onToggled: {
                                    root.model.setEnabled(index, checked);
                                }
                                Layout.preferredWidth: 18
                                Layout.preferredHeight: 18
                                Layout.alignment: Qt.AlignVCenter

                                indicator: Rectangle {
                                    width: 16; height: 16
                                    radius: 3
                                    color: parent.checked
                                           ? PluginTheme.accentA
                                           : PluginTheme.surface
                                    border.color: parent.checked
                                                  ? PluginTheme.accentA
                                                  : PluginTheme.borderSubtle
                                    border.width: 1

                                    Text {
                                        anchors.centerIn: parent
                                        text: "✓"
                                        color: PluginTheme.textOnAccent
                                        font.pixelSize: 11
                                        font.weight: Font.Bold
                                        visible: parent.parent.checked
                                    }
                                }
                            }

                            // Spacer for required params (align with checkbox)
                            Item {
                                visible: model.required
                                Layout.preferredWidth: 18
                                Layout.preferredHeight: 18
                            }

                            // Label
                            Label {
                                text: model.name + (model.required ? " *" : "")
                                color: PluginTheme.textPrimary
                                font.pixelSize: 13
                                Layout.preferredWidth: 120
                                Layout.alignment: Qt.AlignVCenter
                                elide: Text.ElideRight

                                ToolTip.visible: paramLabelMa.containsMouse
                                                 && model.description !== ""
                                ToolTip.text: model.description
                                ToolTip.delay: 500

                                MouseArea {
                                    id: paramLabelMa
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    acceptedButtons: Qt.NoButton
                                }
                            }

                            // Value editor — type-dependent
                            Loader {
                                Layout.fillWidth: true
                                enabled: model.enabled

                                sourceComponent: {
                                    switch (model.paramType) {
                                    case "boolean":
                                        return boolDelegate;
                                    case "number":
                                    case "integer":
                                        return numberDelegate;
                                    case "array":
                                    case "object":
                                        return textAreaDelegate;
                                    default:
                                        return stringDelegate;
                                    }
                                }

                                onLoaded: {
                                    item.paramIndex = index;
                                    item.paramValue = model.value;
                                    item.paramName = model.name;
                                }
                            }
                        }
                    }

                    // Empty state
                    Label {
                        visible: paramRepeater.count === 0
                        text: qsTr("No parameters for this action.")
                        color: PluginTheme.textTertiary
                        font.pixelSize: 13
                        topPadding: PluginTheme.gap
                    }
                }
            }
        }

        // ── Action buttons row ─────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: PluginTheme.gapTight

            Button {
                text: qsTr("▶ Execute")
                enabled: !root.isExecuting
                onClicked: root.executeClicked()

                contentItem: Label {
                    text: parent.text
                    color: PluginTheme.textOnAccent
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                }

                background: Rectangle {
                    implicitHeight: 32
                    implicitWidth: 100
                    color: parent.enabled
                           ? (parent.down ? Qt.darker(PluginTheme.accentA, 1.2)
                                          : PluginTheme.accentA)
                           : PluginTheme.surfaceStrong
                    radius: PluginTheme.radiusSmall / 2

                    Behavior on color {
                        ColorAnimation { duration: PluginTheme.animFast }
                    }
                }
            }

            Button {
                text: qsTr("Clear")
                enabled: !root.isExecuting
                onClicked: root.clearClicked()

                contentItem: Label {
                    text: parent.text
                    color: PluginTheme.textSecondary
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                }

                background: Rectangle {
                    implicitHeight: 32
                    implicitWidth: 72
                    color: parent.down
                           ? PluginTheme.surfaceStrong
                           : PluginTheme.surfaceMuted
                    radius: PluginTheme.radiusSmall / 2
                    border.color: PluginTheme.borderSubtle
                    border.width: 1
                }
            }

            Item { Layout.fillWidth: true }

            ColumnLayout {
                visible: root.executionState !== "idle"
                spacing: 3
                Layout.preferredWidth: 230

                RowLayout {
                    Layout.fillWidth: true
                    spacing: PluginTheme.gapTight

                    Label {
                        Layout.fillWidth: true
                        text: root.progressMessage.length > 0
                              ? root.progressMessage
                              : (root.executionState === "failed"
                                 ? qsTr("Execution failed")
                                 : root.executionState === "succeeded"
                                   ? qsTr("Completed")
                                   : qsTr("Processing…"))
                        color: root.executionState === "failed"
                               ? PluginTheme.danger
                               : root.executionState === "succeeded"
                                 ? PluginTheme.success
                                 : PluginTheme.textSecondary
                        font.pixelSize: 11
                        elide: Text.ElideRight
                    }

                    Label {
                        visible: root.progress > 0
                                 && root.executionState !== "failed"
                        text: Math.round(Math.max(0, Math.min(1, root.progress)) * 100) + "%"
                        color: root.executionState === "succeeded"
                               ? PluginTheme.success
                               : PluginTheme.accentA
                        font.pixelSize: 10
                        font.weight: Font.DemiBold
                    }
                }

                ProgressBar {
                    id: progressBar
                    readonly property bool waitingForProgress:
                        root.isExecuting && root.progress <= 0

                    from: 0
                    to: 1
                    value: Math.max(from, Math.min(to, root.progress))
                    indeterminate: waitingForProgress
                    Layout.fillWidth: true
                    Layout.preferredHeight: 6

                    background: Rectangle {
                        radius: 3
                        color: PluginTheme.surfaceStrong
                    }

                    contentItem: Item {
                        clip: true

                        Rectangle {
                            id: progressFill
                            width: progressBar.waitingForProgress
                                   ? Math.max(parent.width * 0.3, 32)
                                   : parent.width * progressBar.visualPosition
                            height: parent.height
                            radius: 3
                            color: root.executionState === "failed"
                                   ? PluginTheme.danger
                                   : root.executionState === "succeeded"
                                     ? PluginTheme.success
                                     : PluginTheme.accentA
                            x: progressBar.waitingForProgress ? -width : 0

                            Behavior on width {
                                enabled: !progressBar.waitingForProgress
                                NumberAnimation {
                                    duration: PluginTheme.animNormal
                                    easing.type: Easing.OutCubic
                                }
                            }

                            NumberAnimation on x {
                                running: progressBar.waitingForProgress
                                         && progressBar.visible
                                from: -progressFill.width
                                to: progressFill.parent.width
                                duration: 1100
                                loops: Animation.Infinite
                            }
                        }
                    }
                }
            }
        }
    }

    Timer {
        interval: root.executionState === "failed" ? 4000 : 3000
        running: root.executionState === "succeeded"
                 || root.executionState === "failed"
        repeat: false
        onTriggered: root.statusDismissed()
    }

    // ── Inline delegate components ─────────────────────────────────────

    Component {
        id: stringDelegate
        TextField {
            property int paramIndex: -1
            property var paramValue: ""
            property string paramName: ""

            text: paramValue ?? ""
            placeholderText: paramName
            color: PluginTheme.textPrimary
            font.pixelSize: 13
            font.family: PluginTheme.monoFont
            selectionColor: PluginTheme.accentA
            selectedTextColor: PluginTheme.textOnAccent

            background: Rectangle {
                implicitHeight: 30
                color: PluginTheme.surface
                radius: PluginTheme.radiusSmall / 2
                border.color: parent.activeFocus
                              ? PluginTheme.accentA
                              : PluginTheme.borderSubtle
                border.width: 1
            }

            onTextEdited: root.model.setValue(paramIndex, text)
        }
    }

    Component {
        id: numberDelegate
        TextField {
            property int paramIndex: -1
            property var paramValue: 0
            property string paramName: ""

            text: String(paramValue ?? 0)
            placeholderText: paramName
            color: PluginTheme.textPrimary
            font.pixelSize: 13
            font.family: PluginTheme.monoFont
            validator: DoubleValidator {}
            selectionColor: PluginTheme.accentA
            selectedTextColor: PluginTheme.textOnAccent

            background: Rectangle {
                implicitHeight: 30
                color: PluginTheme.surface
                radius: PluginTheme.radiusSmall / 2
                border.color: parent.activeFocus
                              ? PluginTheme.accentA
                              : PluginTheme.borderSubtle
                border.width: 1
            }

            onTextEdited: root.model.setValue(paramIndex, text)
        }
    }

    Component {
        id: boolDelegate
        CheckBox {
            property int paramIndex: -1
            property var paramValue: false
            property string paramName: ""

            checked: paramValue ?? false
            text: checked ? "true" : "false"

            indicator: Rectangle {
                y: parent.height / 2 - height / 2
                width: 16; height: 16
                radius: 3
                color: parent.checked
                       ? PluginTheme.accentA
                       : PluginTheme.surface
                border.color: parent.checked
                              ? PluginTheme.accentA
                              : PluginTheme.borderSubtle
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "✓"
                    color: PluginTheme.textOnAccent
                    font.pixelSize: 11
                    font.weight: Font.Bold
                    visible: parent.parent.checked
                }
            }

            contentItem: Label {
                text: parent.text
                color: PluginTheme.textPrimary
                font.pixelSize: 13
                font.family: PluginTheme.monoFont
                leftPadding: 22
                verticalAlignment: Text.AlignVCenter
            }

            onToggled: root.model.setValue(paramIndex, checked)
        }
    }

    Component {
        id: textAreaDelegate
        TextArea {
            property int paramIndex: -1
            property var paramValue: ""
            property string paramName: ""

            text: String(paramValue ?? "")
            placeholderText: paramName
            wrapMode: TextEdit.Wrap
            color: PluginTheme.textPrimary
            font.pixelSize: 13
            font.family: PluginTheme.monoFont
            selectionColor: PluginTheme.accentA
            selectedTextColor: PluginTheme.textOnAccent

            implicitHeight: Math.max(60, contentHeight + topPadding + bottomPadding)

            background: Rectangle {
                color: PluginTheme.surface
                radius: PluginTheme.radiusSmall / 2
                border.color: parent.activeFocus
                              ? PluginTheme.accentA
                              : PluginTheme.borderSubtle
                border.width: 1
            }

            onTextChanged: {
                if (activeFocus) {
                    root.model.setValue(paramIndex, text);
                }
            }
        }
    }
}
