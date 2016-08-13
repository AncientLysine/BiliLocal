import QtQuick 2.7
import QtQuick.Controls 2.0
import BiliLocal 1.0

Drawer {
    edge: Qt.RightEdge
    height: parent.height
    dragMargin : 25

    ListView {
        id: danmakuView
        anchors.fill: parent
        model: LocalApp.Danmaku
        delegate: Rectangle {
            color: index % 2 != 0 ? "white" : "lightgray"
            width: parent.width
            height: commenRow.height

            Row {
                id : commenRow
                spacing: parent.width / 24

                Text {
                    text : {
                        if (block){
                            return qsTr("Blocked");
                        }
                        else{
                            var str = "";
                            var t = Math.floor(time / 1000);
                            if (t < 0){
                                str += "-";
                                t = -t;
                            }
                            var sec = Math.floor(t % 60);
                            var min = Math.floor(t / 60);
                            str += min < 10 ? "0" + min : min;
                            str += ":"
                            str += sec < 10 ? "0" + sec : sec;
                            return str;
                        }
                    }
                }

                Text {
                    maximumLineCount: 1
                    text : string
                }
            }
        }
    }
}
