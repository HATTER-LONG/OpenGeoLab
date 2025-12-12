pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

/**
 * @file RibbonFileMenu.qml
 * @brief File menu popup for Ribbon toolbar (Office-style backstage)
 *
 * Displays file operations (New, Import, Export, Options) in a left panel
 * with recent files list on the right.
 */
Popup {
    id: fileMenu

    // Signals for file operations
    signal newFile
    signal openFile
    signal saveFile
    signal saveAsFile
    signal importModel
    signal exportModel
    signal replayFile
    signal showOptions
    signal exitApp

    // ========================================================================
    // Dark theme colors (fixed)
    // ========================================================================
    readonly property color accentColor: "#0d6efd"
    readonly property color hoverColor: "#3a3f4b"
    readonly property color menuBackgroundColor: "#1a1d24"
    readonly property color contentBackgroundColor: "#252830"
    readonly property color textColor: "#e1e1e1"
    readonly property color textColorDim: "#a0a0a0"
    readonly property color separatorColor: "#3a3f4b"

    width: 700
    height: 500
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: 0

    background: Rectangle {
        color: fileMenu.contentBackgroundColor
        border.color: fileMenu.separatorColor
        border.width: 1
    }

    contentItem: Row {
        spacing: 0

        // Left menu panel
        Rectangle {
            width: 200
            height: parent.height
            color: fileMenu.menuBackgroundColor

            Column {
                anchors.fill: parent
                anchors.topMargin: 10
                spacing: 0

                FileMenuItem {
                    iconText: "📄"
                    text: "New"
                    shortcut: "Ctrl+N"
                    onClicked: {
                        fileMenu.newFile();
                        fileMenu.close();
                    }
                }

                FileMenuItem {
                    iconText: "📥"
                    text: "Import"
                    shortcut: "Ctrl+I"
                    onClicked: {
                        fileMenu.importModel();
                        fileMenu.close();
                    }
                }

                FileMenuItem {
                    iconText: "📤"
                    text: "Export"
                    shortcut: "Ctrl+E"
                    onClicked: {
                        fileMenu.exportModel();
                        fileMenu.close();
                    }
                }

                // Spacer
                Item {
                    width: 1
                    height: 30
                }

                // Separator
                Rectangle {
                    width: parent.width - 20
                    height: 1
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: Qt.rgba(1, 1, 1, 0.3)
                }

                Item {
                    width: 1
                    height: 10
                }

                FileMenuItem {
                    iconText: "⚙"
                    text: "Options"
                    onClicked: {
                        fileMenu.showOptions();
                        fileMenu.close();
                    }
                }
            }
        }

        // Right content panel - Recent Files
        Rectangle {
            width: parent.width - 200
            height: parent.height
            color: fileMenu.contentBackgroundColor

            Column {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 10

                Text {
                    text: "Recent Files"
                    font.pixelSize: 18
                    font.bold: true
                    color: fileMenu.textColor
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: fileMenu.separatorColor
                }

                // Recent files list
                ListView {
                    id: recentFilesList
                    width: parent.width
                    height: parent.height - 50
                    clip: true
                    spacing: 2

                    model: ListModel {
                        id: recentFilesModel
                        // Example items - should be populated dynamically
                        ListElement {
                            fileName: "modelY_biw_left_withoutWheels.stp"
                        }
                        ListElement {
                            fileName: "archerpre.0245.py"
                        }
                        ListElement {
                            fileName: "ventDr_mod.stp"
                        }
                        ListElement {
                            fileName: "as1-oc-214.stp"
                        }
                        ListElement {
                            fileName: "test.brep"
                        }
                        ListElement {
                            fileName: "test2.brep"
                        }
                        ListElement {
                            fileName: "ck.brep"
                        }
                        ListElement {
                            fileName: "chuangk.brep"
                        }
                        ListElement {
                            fileName: "chuangkuang_update.arp"
                        }
                        ListElement {
                            fileName: "ne.brep"
                        }
                        ListElement {
                            fileName: "ce.brep"
                        }
                        ListElement {
                            fileName: "sample1_issue.stp"
                        }
                    }

                    delegate: Rectangle {
                        id: recentFileDelegate
                        required property int index
                        required property string fileName

                        width: recentFilesList.width
                        height: 28
                        color: recentFileMouseArea.containsMouse ? fileMenu.hoverColor : "transparent"
                        radius: 3

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            spacing: 10

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: (recentFileDelegate.index + 1).toString()
                                font.pixelSize: 12
                                color: fileMenu.accentColor
                                width: 20
                            }

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: recentFileDelegate.fileName
                                font.pixelSize: 12
                                color: fileMenu.textColor
                                elide: Text.ElideMiddle
                                width: parent.width - 40
                            }
                        }

                        MouseArea {
                            id: recentFileMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                console.log("Open recent file:", recentFileDelegate.fileName);
                                fileMenu.close();
                            }
                        }
                    }
                }
            }
        }
    }

    // Function to update recent files
    function setRecentFiles(files: list<string>): void {
        recentFilesModel.clear();
        for (let file of files) {
            recentFilesModel.append({
                fileName: file
            });
        }
    }
}
