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
    src/Info.cpp \
    src/Search.cpp \
    src/Utils.cpp \
    src/Shield.cpp \
    src/Config.cpp \
    src/Editor.cpp \
    src/Cookie.cpp \
    src/Graphic.cpp \
    src/Panel.cpp \
    src/Platform.cpp \
    src/Render.cpp

HEADERS  += \
    src/Interface.h \
    src/Danmaku.h \
    src/VPlayer.h \
    src/Menu.h \
    src/Info.h \
    src/Search.h \
    src/Utils.h \
    src/Shield.h \
    src/Config.h \
    src/Editor.h \
    src/Cookie.h \
    src/Graphic.h \
    src/Panel.h \
    src/Platform.h \
    src/Render.h

LIBS += \
    -lvlc \
    -lvlccore

RESOURCES += \
    res/Res.qrc

TRANSLATIONS += \
    trans/zh_CN.ts \
    trans/zh_HK.ts \
    trans/zh_TW.ts

win32{
RC_ICONS = BiliLocal.ico
}
