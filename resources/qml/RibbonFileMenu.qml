pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

/**
 * @brief File menu popup for Ribbon toolbar (Office-style backstage)
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

    property color accentColor: "#0078D4"
    property color hoverColor: "#E5F1FB"
    property color menuBackgroundColor: "#2B579A"
    property color contentBackgroundColor: "#FFFFFF"

    width: 700
    height: 500
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: 0

    background: Rectangle {
        color: fileMenu.contentBackgroundColor
        border.color: "#D1D1D1"
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
                    onClicked: {
                        fileMenu.newFile();
                        fileMenu.close();
                    }
                }

                FileMenuItem {
                    iconText: "📂"
                    text: "Open"
                    shortcut: "Ctrl+O"
                    onClicked: {
                        fileMenu.openFile();
                        fileMenu.close();
                    }
                }

                FileMenuItem {
                    iconText: "📥"
                    text: "Import"
                    onClicked: {
                        fileMenu.importModel();
                        fileMenu.close();
                    }
                }

                FileMenuItem {
                    iconText: "💾"
                    text: "Save"
                    onClicked: {
                        fileMenu.saveFile();
                        fileMenu.close();
                    }
                }

                FileMenuItem {
                    iconText: "💾"
                    text: "Save As"
                    onClicked: {
                        fileMenu.saveAsFile();
                        fileMenu.close();
                    }
                }

                FileMenuItem {
                    iconText: "▶"
                    text: "Replay"
                    onClicked: {
                        fileMenu.replayFile();
                        fileMenu.close();
                    }
                }

                FileMenuItem {
                    iconText: "📤"
                    text: "Export"
                    hasSubmenu: true
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

                FileMenuItem {
                    iconText: "🚪"
                    text: "Exit"
                    onClicked: {
                        fileMenu.exitApp();
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
                    color: "#333333"
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "#E0E0E0"
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
                        color: recentFileMouseArea.containsMouse ? "#E5F1FB" : "transparent"
                        radius: 3

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            spacing: 10

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: (recentFileDelegate.index + 1).toString()
                                font.pixelSize: 12
                                color: "#0078D4"
                                width: 20
                            }

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: recentFileDelegate.fileName
                                font.pixelSize: 12
                                color: "#333333"
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
