import QtQuick 2.5
import QtQuick.Window 2.2
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2
import BiliLocal 1.0

Window {
    id: mainWindow
    visible: true
    width: 960
    height: 540
    title: "BiliLocal"

    Timer {
        interval: 1
        running: true
        repeat: true
        onTriggered: mainWindow.update();
    }

    Menu {
        id: contextMenu
        MenuItem {
            text: qsTr("Play")
            onTriggered: LocalApp.Player.play();
            shortcut: LocalApp.Config.getVariant("/Shortcut/Play", "Space")
        }
        MenuItem {
            text: qsTr("Stop")
            onTriggered: LocalApp.Player.stop(true);
        }
        MenuItem {
            text: qsTr("Open")
            onTriggered: openDialog.open();
        }
        MenuItem {
            text: qsTr("Load")
            onTriggered: loadDialog.open();
        }
        MenuItem {
            text: qsTr("Prefer")
            onTriggered: Qt.quit();
            shortcut: LocalApp.Config.getVariant("/Shortcut/Conf", "Ctrl+I")
        }
        MenuItem {
            text: qsTr("Quit")
            onTriggered: Qt.quit();
            shortcut: LocalApp.Config.getVariant("/Shortcut/Quit", "Ctrl+Q")
        }
    }

    MouseArea {
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        anchors.fill: parent
        onClicked: contextMenu.popup();
    }

    FileDialog {
        id: openDialog
        title: qsTr("Open Media")
        onAccepted: {
            var u = openDialog.fileUrls[0];
            LocalApp.Player.setMedia(u);
        }
    }

    FileDialog {
        id: loadDialog
        title: qsTr("Load Danmaku")
        onAccepted: {
            var u = loadDialog.fileUrls[0];
            LocalApp.Load.loadDanmaku(u);
        }
    }
}
