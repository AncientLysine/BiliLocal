import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import BiliLocal 1.0

Drawer {
    edge: Qt.LeftEdge
    height: parent.height
    dragMargin : 25

    GridLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: parent.width / 32

        TextInput {
            Layout.row: 0
            Layout.column: 0
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            text: LocalApp.Player.media
        }

        Button {
            Layout.row: 0
            Layout.column: 1
            text : qsTr("Open")
            onClicked: openDialog.open()
        }

        TextInput {
            id: loadText
            Layout.row: 1
            Layout.column: 0
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
        }

        Button {
            Layout.row: 1
            Layout.column: 1
            text : localDanmakuSwitch.checked ? qsTr("Open") : qsTr("Load")
            onClicked: {
                if (localDanmakuSwitch.checked) {
                    loadDialog.open();
                }
                else {
                    LocalApp.Load.loadDanmaku(loadText.text);
                }
            }
        }

        TextInput {
            Layout.row: 2
            Layout.column: 0
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
        }

        Button {
            Layout.row: 2
            Layout.column: 1
            text : qsTr("Seek")
        }

        Text {
            Layout.row: 3
            Layout.column: 0
            text: qsTr("Danmaku Alpha")
        }

        Slider {
            Layout.row: 4
            Layout.column: 0
            Layout.columnSpan: 2
            Layout.fillWidth: true
            value : LocalApp.Config.getVariant("/Danmaku/Alpha", 100) / 100
            onValueChanged: {
                LocalApp.Config.setVariant("/Danmaku/Alpha", value * 100)
            }
        }

        Text {
            Layout.row: 5
            Layout.column: 0
            text: qsTr("Local Danmaku")
        }

        Switch {
            id: localDanmakuSwitch
            Layout.row: 5
            Layout.column: 1
            Layout.alignment: Qt.AlignHCenter
            checked : LocalApp.Config.getVariant("/Danmaku/Local", false)
            onCheckedChanged: {
                LocalApp.Config.setVariant("/Danmaku/Local", checked)
            }
        }

        Text {
            Layout.row: 6
            Layout.column: 0
            text: qsTr("Protect Sub")
        }

        Switch {
            id: protectSubSwitch
            Layout.row: 6
            Layout.column: 1
            Layout.alignment: Qt.AlignHCenter
            checked : LocalApp.Config.getVariant("/Danmaku/Protect", false)
            onCheckedChanged: {
                LocalApp.Config.setVariant("/Danmaku/Protect", checked)
            }
        }

        Text {
            Layout.row: 7
            Layout.column: 0
            text: qsTr("Loop Playback")
        }

        Switch {
            id: loopPlaybackSwitch
            Layout.row: 7
            Layout.column: 1
            Layout.alignment: Qt.AlignHCenter
            checked : LocalApp.Config.getVariant("/Player/Loop", false)
            onCheckedChanged: {
                LocalApp.Config.setVariant("/Player/Loop", checked)
            }
        }
    }
}
