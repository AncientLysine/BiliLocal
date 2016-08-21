import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Dialogs 1.2
import BiliLocal 1.0

ApplicationWindow {
    title: "BiliLocal"
    width: 960
    height: 540
    visible: true

    Home {
        id: homeItem
    }

    FileDialog {
        id: openDialog
        title: qsTr("Open Media")
        onAccepted: {
            var u = openDialog.fileUrls[0];
            u = u ? u : "/sdcard/Movies/Sicily.mp4";
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

    Connections {
        target: Qt.application
        onStateChanged: {
            if (LocalApp.Player.state === APlayer.Play) {
                LocalApp.Player.play();
            }
        }
    }
}
