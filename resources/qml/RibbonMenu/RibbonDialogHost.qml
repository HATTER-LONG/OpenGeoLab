pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

Item {
    id: host
    anchors.fill: parent

    // Optional sizing for the shell popup.
    property int popupWidth: 440
    property int popupHeight: 260

    // For Component-based dialogs.
    property var _createdObject: null

    // Single floating dialog at a time (keeps UX simple and predictable).
    // URL-based (kept for compatibility).
    function open(qmlUrl, properties, options): void {
        openUrl(qmlUrl, properties, options);
    }

    function openUrl(qmlUrl, properties, options): void {
        _applyOptions(options);
        _destroyCreated();
        contentLoader.setSource(qmlUrl, properties || ({}));
        popup.open();
    }

    function openComponent(component, properties, options): void {
        if (!component || !component.createObject) {
            console.error("[RibbonDialogHost] Invalid component");
            return;
        }

        _applyOptions(options);
        contentLoader.active = false;
        contentLoader.source = "";
        _destroyCreated();

        _createdObject = component.createObject(contentRoot, properties || ({}));
        if (!_createdObject) {
            console.error("[RibbonDialogHost] Failed to create dialog object");
            return;
        }

        popup.open();
    }

    function close(): void {
        popup.close();
    }

    function _applyOptions(options): void {
        if (!options)
            return;
        if (options.width !== undefined)
            host.popupWidth = options.width;
        if (options.height !== undefined)
            host.popupHeight = options.height;
    }

    function _destroyCreated(): void {
        if (_createdObject && _createdObject.destroy)
            _createdObject.destroy();
        _createdObject = null;
    }

    Popup {
        id: popup
        modal: false
        focus: true
        closePolicy: Popup.CloseOnEscape

        anchors.centerIn: parent
        padding: 0

        width: host.popupWidth
        height: host.popupHeight

        onClosed: {
            contentLoader.active = false;
            contentLoader.source = "";
            host._destroyCreated();
        }

        contentItem: Item {
            anchors.fill: parent

            Item {
                id: contentRoot
                anchors.fill: parent
            }

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

            Connections {
                target: host._createdObject
                ignoreUnknownSignals: true

                function onCloseRequested() {
                    popup.close();
                }
            }
        }

        onOpened: {
            contentLoader.active = true;
        }
    }
}
