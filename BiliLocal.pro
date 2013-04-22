QT += \
    core \
    gui \
    widgets \
    network

TARGET = BiliLocal

TEMPLATE = app

CONFIG += c++11

SOURCES += \
    src/Interface.cpp \
    src/Local.cpp \
    src/Danmaku.cpp \
    src/VPlayer.cpp \
    src/Menu.cpp \
    src/State.cpp

HEADERS  += \
    src/Interface.h \
    src/Danmaku.h \
    src/VPlayer.h \
    src/Menu.h \
    src/State.h

LIBS += -L./\
    -lvlc \
    -lvlccore \
    -lavutil \
    -lswscale

INCLUDEPATH += include/

RESOURCES += \
    res/Res.qrc \
    trans/Trans.qrc

TRANSLATIONS = \
    trans/zh_CN.ts

win32 {

RC_FILE = \
    Icon.rc

}
