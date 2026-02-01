/**
 * @file ViewToolbar.qml
 * @brief Viewport control toolbar with view preset buttons
 *
 * Provides buttons for camera view presets (Fit, Front, Back, Top, Bottom, Left, Right)
 * with icons. Positioned at the top-right of the viewport.
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import OpenGeoLab 1.0

Rectangle {
    id: viewToolbar

    implicitWidth: buttonRow.implicitWidth + 18
    implicitHeight: 34
    radius: 8
    color: Qt.rgba(Theme.surfaceAlt.r, Theme.surfaceAlt.g, Theme.surfaceAlt.b, 0.88)
    border.width: 1
    border.color: Theme.border

    /**
     * @brief Send a render action request
     * @param actionName Render action id
     * @param payload JSON payload object (merged into request)
     */
    function send(actionName, payload) {
        const requestObj = payload || {};
        requestObj.action = actionName;
        requestObj._meta = {
            silent: true
        };
        BackendService.request("RenderService", JSON.stringify(requestObj));
    }

    RowLayout {
        id: buttonRow
        anchors.centerIn: parent
        spacing: 3

        // Fit button
        ViewToolButton {
            iconSource: "qrc:/opengeolab/resources/icons/view_fit.svg"
            toolTipText: qsTr("Fit to scene (F)")
            onClicked: viewToolbar.send("ViewPortControl", {
                view_ctrl: {
                    fit: true
                }
            })
        }

        // Separator
        Rectangle {
            width: 1
            height: 20
            color: Theme.border
            Layout.alignment: Qt.AlignVCenter
        }

        // Front = 0,
        // Back = 1,
        // Left = 2,
        // Right = 3,
        // Top = 4,
        // Bottom = 5
        // Front view

        ViewToolButton {
            iconSource: "qrc:/opengeolab/resources/icons/view_front.svg"
            toolTipText: qsTr("Front view")
            onClicked: viewToolbar.send("ViewPortControl", {
                view_ctrl: {
                    view: 0
                }
            })
        }

        // Back view
        ViewToolButton {
            iconSource: "qrc:/opengeolab/resources/icons/view_back.svg"
            toolTipText: qsTr("Back view")
            onClicked: viewToolbar.send("ViewPortControl", {
                view_ctrl: {
                    view: 1
                }
            })
        }

        // Top view
        ViewToolButton {
            iconSource: "qrc:/opengeolab/resources/icons/view_top.svg"
            toolTipText: qsTr("Top view")
            onClicked: viewToolbar.send("ViewPortControl", {
                view_ctrl: {
                    view: 4
                }
            })
        }

        // Bottom view
        ViewToolButton {
            iconSource: "qrc:/opengeolab/resources/icons/view_bottom.svg"
            toolTipText: qsTr("Bottom view")
            onClicked: viewToolbar.send("ViewPortControl", {
                view_ctrl: {
                    view: 5
                }
            })
        }

        // Left view
        ViewToolButton {
            iconSource: "qrc:/opengeolab/resources/icons/view_left.svg"
            toolTipText: qsTr("Left view")
            onClicked: viewToolbar.send("ViewPortControl", {
                view_ctrl: {
                    view: 2
                }
            })
        }

        // Right view
        ViewToolButton {
            iconSource: "qrc:/opengeolab/resources/icons/view_right.svg"
            toolTipText: qsTr("Right view")
            onClicked: viewToolbar.send("ViewPortControl", {
                view_ctrl: {
                    view: 3
                }
            })
        }
    }
}
