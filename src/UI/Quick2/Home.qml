import QtQuick 2.7
import QtQuick.Controls 2.0
import BiliLocal 1.0
import "./" as UI

Item {
    anchors.fill: parent

    UI.Menu{
        id: menuDrawer
        width: 240
    }

    UI.Info{
        id: infoDrawer
        width: 240
    }

    Menu {
        id: contextMenu
        modal : true
        dim : false
        x: mouseArea.mouseX
        y: mouseArea.mouseY

        MenuItem {
            text: qsTr("Play")
            onTriggered: LocalApp.Player.play()
        }

        MenuItem {
            text: qsTr("Stop")
            onTriggered: LocalApp.Player.stop(true)
        }

        MenuItem {
            text: qsTr("Open")
            onTriggered: openDialog.open()
        }

        MenuItem {
            text: qsTr("Load")
            onTriggered: {
                if (LocalApp.Config.getVariant("/Danmaku/Local", false)) {
                    loadDialog.open();
                }
                else {
                    menuDrawer.open();
                }
            }
        }

        MenuItem {
            text: qsTr("Prefer")
        }

        MenuItem {
            text: qsTr("Quit")
            onTriggered: Qt.quit()
        }
    }

    MouseArea {
        id: mouseArea
        acceptedButtons: Qt.AllButtons
        anchors.fill: parent
        onClicked: contextMenu.open()
    }
}
