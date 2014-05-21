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
    src/Graphic.cpp \
    src/Post.cpp \
    src/Render.cpp \
    src/Next.cpp \
    src/Load.cpp \
    src/Plugin.cpp

HEADERS  += \
    src/Interface.h \
    src/Local.h \
    src/Danmaku.h \
    src/VPlayer.h \
    src/Menu.h \
    src/Info.h \
    src/Search.h \
    src/Utils.h \
    src/Shield.h \
    src/Config.h \
    src/Editor.h \
    src/Graphic.h \
    src/Post.h \
    src/Render.h \
    src/Next.h \
    src/Load.h \
    src/Plugin.h

LIBS += \
    -lvlc \
    -lvlccore \
    -lswscale \
    -lavutil

RESOURCES += \
    res/Res.qrc

TRANSLATIONS += \
    res/zh_CN.ts \
    res/zh_TW.ts

win32{
RC_ICONS = BiliLocal.ico
}
