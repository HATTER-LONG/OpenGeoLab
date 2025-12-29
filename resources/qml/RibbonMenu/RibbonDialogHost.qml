pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

Item {
    id: host
    anchors.fill: parent

    // Single floating dialog at a time (keeps UX simple and predictable).
    function open(qmlUrl, properties): void {
        contentLoader.setSource(qmlUrl, properties || ({}));
        popup.open();
    }

    function close(): void {
        popup.close();
    }

    Popup {
        id: popup
        modal: false
        focus: true
        closePolicy: Popup.CloseOnEscape

        anchors.centerIn: parent
        padding: 0

        width: 440
        height: 260

        onClosed: {
            contentLoader.active = false;
            contentLoader.source = "";
        }

        contentItem: Item {
            anchors.fill: parent

            Loader {
                id: contentLoader
                anchors.fill: parent
                active: false
                asynchronous: true

                onLoaded: {
                    // no-op
                }

                onStatusChanged: {
                    if (status === Loader.Error) {
                        console.error("[RibbonDialogHost] Failed to load dialog:", source);
                        popup.close();
                    }
                }

                Connections {
                    target: contentLoader.item
                    ignoreUnknownSignals: true

                    function onCloseRequested() {
                        popup.close();
                    }
                }
            }
        }

        onOpened: {
            contentLoader.active = true;
        }
    }
}
