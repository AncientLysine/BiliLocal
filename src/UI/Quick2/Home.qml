import QtQuick 2.7
import QtQuick.Controls 2.0
import BiliLocal 1.0
import "./" as UI

Item {
    anchors.fill: parent

    UI.Menu{
        id: menuDrawer
        width: Math.min(parent.width, Math.max(parent.width, parent.height) / 2)
    }

    UI.Info{
        id: infoDrawer
        width: menuDrawer.width
    }

    Menu {
        id: contextMenu
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
            onTriggered: loadDialog.open()
        }

        MenuItem {
            text: qsTr("Prefer")
            onTriggered: Qt.quit()
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
