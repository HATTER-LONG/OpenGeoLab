pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Window
import Qt5Compat.GraphicalEffects
import "../theme"

Item {
    id: iconRoot

    required property AppTheme theme
    property string iconKind: "menu"
    property color primaryColor: theme.accentA
    property bool useThemeContrast: true

    implicitWidth: 24
    implicitHeight: 24

    readonly property color overlayColor: useThemeContrast ? theme.textPrimary : primaryColor
    readonly property int pixelWidth: Math.max(1, Math.round(width * Screen.devicePixelRatio))
    readonly property int pixelHeight: Math.max(1, Math.round(height * Screen.devicePixelRatio))

    readonly property string iconFileName: iconKind.length > 0 ? iconKind + ".svg" : ""
    readonly property url iconUrl: iconFileName.length > 0 ? Qt.resolvedUrl("../../icons/" + iconFileName) : ""

    Image {
        id: iconImage

        anchors.fill: parent
        source: iconRoot.iconUrl
        fillMode: Image.PreserveAspectFit
        smooth: true
        antialiasing: true
        cache: true
        visible: false
        sourceSize.width: iconRoot.pixelWidth
        sourceSize.height: iconRoot.pixelHeight
    }

    ColorOverlay {
        anchors.fill: iconImage
        source: iconImage
        color: iconRoot.overlayColor
        visible: iconImage.status === Image.Ready && iconRoot.iconUrl.toString().length > 0
    }

    Rectangle {
        anchors.fill: parent
        radius: width / 4
        color: Qt.rgba(iconRoot.overlayColor.r, iconRoot.overlayColor.g, iconRoot.overlayColor.b, 0.12)
        border.width: 1
        border.color: Qt.rgba(iconRoot.overlayColor.r, iconRoot.overlayColor.g, iconRoot.overlayColor.b, 0.32)
        visible: iconImage.status !== Image.Ready && iconRoot.iconKind.length > 0
    }
}
