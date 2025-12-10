/**
 * CreateBoxDialog.qml
 * Dialog for creating a box with custom dimensions
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root

    title: "Create Box"
    modal: true

    // Output values
    property real boxWidth: 1.0
    property real boxHeight: 1.0
    property real boxDepth: 1.0

    // Signal emitted when user confirms box creation
    signal boxCreated(real width, real height, real depth)

    width: 320
    height: 280

    background: Rectangle {
        color: "#252830"
        radius: 8
        border.color: "#3a3f4b"
        border.width: 1
    }

    header: Rectangle {
        color: "#1a1d23"
        height: 40
        radius: 8

        // Bottom corners should be square
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 8
            color: parent.color
        }

        Text {
            anchors.centerIn: parent
            text: root.title
            color: "#e1e1e1"
            font.pixelSize: 14
            font.bold: true
        }
    }

    contentItem: ColumnLayout {
        spacing: 16

        // Width input
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: "Width (X):"
                color: "#e1e1e1"
                font.pixelSize: 13
                Layout.preferredWidth: 80
            }

            TextField {
                id: widthField
                Layout.fillWidth: true
                text: root.boxWidth.toFixed(2)
                color: "#e1e1e1"
                placeholderTextColor: "#666"
                font.pixelSize: 13
                horizontalAlignment: Text.AlignRight
                validator: DoubleValidator {
                    bottom: 0.001
                    decimals: 3
                }

                background: Rectangle {
                    color: "#1e2127"
                    radius: 4
                    border.color: widthField.activeFocus ? "#0d6efd" : "#3a3f4b"
                    border.width: 1
                }

                onTextChanged: {
                    let val = parseFloat(text);
                    if (!isNaN(val) && val > 0) {
                        root.boxWidth = val;
                    }
                }
            }
        }

        // Height input
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: "Height (Y):"
                color: "#e1e1e1"
                font.pixelSize: 13
                Layout.preferredWidth: 80
            }

            TextField {
                id: heightField
                Layout.fillWidth: true
                text: root.boxHeight.toFixed(2)
                color: "#e1e1e1"
                placeholderTextColor: "#666"
                font.pixelSize: 13
                horizontalAlignment: Text.AlignRight
                validator: DoubleValidator {
                    bottom: 0.001
                    decimals: 3
                }

                background: Rectangle {
                    color: "#1e2127"
                    radius: 4
                    border.color: heightField.activeFocus ? "#0d6efd" : "#3a3f4b"
                    border.width: 1
                }

                onTextChanged: {
                    let val = parseFloat(text);
                    if (!isNaN(val) && val > 0) {
                        root.boxHeight = val;
                    }
                }
            }
        }

        // Depth input
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: "Depth (Z):"
                color: "#e1e1e1"
                font.pixelSize: 13
                Layout.preferredWidth: 80
            }

            TextField {
                id: depthField
                Layout.fillWidth: true
                text: root.boxDepth.toFixed(2)
                color: "#e1e1e1"
                placeholderTextColor: "#666"
                font.pixelSize: 13
                horizontalAlignment: Text.AlignRight
                validator: DoubleValidator {
                    bottom: 0.001
                    decimals: 3
                }

                background: Rectangle {
                    color: "#1e2127"
                    radius: 4
                    border.color: depthField.activeFocus ? "#0d6efd" : "#3a3f4b"
                    border.width: 1
                }

                onTextChanged: {
                    let val = parseFloat(text);
                    if (!isNaN(val) && val > 0) {
                        root.boxDepth = val;
                    }
                }
            }
        }

        // Spacer
        Item {
            Layout.fillHeight: true
        }

        // Info text
        Text {
            text: "Box will be centered at origin"
            color: "#888"
            font.pixelSize: 11
            Layout.alignment: Qt.AlignHCenter
        }
    }

    footer: Rectangle {
        color: "#1a1d23"
        height: 50
        radius: 8

        // Top corners should be square
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 8
            color: parent.color
        }

        Row {
            anchors.centerIn: parent
            spacing: 16

            Button {
                id: okButton
                text: "OK"
                width: 80
                height: 32

                contentItem: Text {
                    text: okButton.text
                    color: "#e1e1e1"
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    color: okButton.down ? "#0b5ed7" : (okButton.hovered ? "#1a6ed8" : "#0d6efd")
                    radius: 4
                }

                onClicked: {
                    root.accept();
                }
            }

            Button {
                id: cancelButton
                text: "Cancel"
                width: 80
                height: 32

                contentItem: Text {
                    text: cancelButton.text
                    color: "#e1e1e1"
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    color: cancelButton.down ? "#3a3f4b" : (cancelButton.hovered ? "#3a3f4b" : "#2a2e35")
                    radius: 4
                    border.color: "#3a3f4b"
                    border.width: 1
                }

                onClicked: {
                    root.reject();
                }
            }
        }
    }

    onAccepted: {
        root.boxCreated(root.boxWidth, root.boxHeight, root.boxDepth);
    }

    // Reset values when opening
    onAboutToShow: {
        widthField.text = root.boxWidth.toFixed(2);
        heightField.text = root.boxHeight.toFixed(2);
        depthField.text = root.boxDepth.toFixed(2);
    }
}
