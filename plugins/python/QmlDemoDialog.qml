import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    title: "OpenGeoLab QML Demo Plugin"
    width: 560
    height: 480
    visible: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Label {
            text: "QML Demo Plugin"
            font.pixelSize: 18
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        GroupBox {
            title: "Points Input"
            Layout.fillWidth: true

            GridLayout {
                columns: 4
                anchors.fill: parent
                columnSpacing: 8
                rowSpacing: 8

                Label {
                    text: "X"
                }

                Label {
                    text: "Y"
                }

                Label {
                    text: "Z"
                }

                Label {
                    text: ""
                }

                TextField {
                    id: x1
                    placeholderText: "1.0"
                    Layout.fillWidth: true
                }

                TextField {
                    id: y1
                    placeholderText: "2.0"
                    Layout.fillWidth: true
                }

                TextField {
                    id: z1
                    placeholderText: "3.0"
                    Layout.fillWidth: true
                }

                Label {
                    text: "Point 1"
                }

                TextField {
                    id: x2
                    placeholderText: "-5.0"
                    Layout.fillWidth: true
                }

                TextField {
                    id: y2
                    placeholderText: "10.0"
                    Layout.fillWidth: true
                }

                TextField {
                    id: z2
                    placeholderText: "0.0"
                    Layout.fillWidth: true
                }

                Label {
                    text: "Point 2"
                }

                TextField {
                    id: x3
                    placeholderText: "100.0"
                    Layout.fillWidth: true
                }

                TextField {
                    id: y3
                    placeholderText: "-50.0"
                    Layout.fillWidth: true
                }

                TextField {
                    id: z3
                    placeholderText: "25.0"
                    Layout.fillWidth: true
                }

                Label {
                    text: "Point 3"
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: "Set Points → C++"
                Layout.fillWidth: true

                onClicked: {
                    const points = [];

                    function addPoint(xField, yField, zField) {
                        const x = parseFloat(xField.text || xField.placeholderText);
                        const y = parseFloat(yField.text || yField.placeholderText);
                        const z = parseFloat(zField.text || zField.placeholderText);

                        if (!isNaN(x) && !isNaN(y) && !isNaN(z)) {
                            points.push({
                                "x": x,
                                "y": y,
                                "z": z
                            });
                        }
                    }

                    addPoint(x1, y1, z1);
                    addPoint(x2, y2, z2);
                    addPoint(x3, y3, z3);

                    const requestJson = JSON.stringify({
                        "action": "geometry.set_points",
                        "payload": {
                            "points": points
                        }
                    });

                    bridge.process_request(requestJson);
                }
            }

            Button {
                text: "Get Stored BBox ← C++"
                Layout.fillWidth: true

                onClicked: {
                    const requestJson = JSON.stringify({
                        "action": "geometry.get_stored_bbox"
                    });
                    bridge.process_request(requestJson);
                }
            }

            Button {
                text: "Compute Random BBox"
                Layout.fillWidth: true

                onClicked: {
                    const requestJson = JSON.stringify({
                        "action": "geometry.bounding_box",
                        "payload": {
                            "pointCount": 1000000
                        }
                    });
                    bridge.process_request(requestJson);
                }
            }
        }

        GroupBox {
            title: "Response"
            Layout.fillWidth: true
            Layout.fillHeight: true

            ScrollView {
                anchors.fill: parent

                TextArea {
                    id: resultArea
                    readOnly: true
                    placeholderText: "Results will appear here."
                    wrapMode: TextArea.Wrap
                }
            }
        }
    }

    Connections {
        target: bridge

        function onResponseReady(responseJson) {
            try {
                const parsed = JSON.parse(responseJson);
                resultArea.text = JSON.stringify(parsed, null, 2);
            } catch (error) {
                resultArea.text = responseJson;
            }
        }
    }
}
