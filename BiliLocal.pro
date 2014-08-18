QT += \
    core \
    gui \
    network \
    widgets

TARGET = BiliLocal

TEMPLATE = app

CONFIG += c++11

SOURCES += \
    src/APlayer.cpp \
    src/Config.cpp \
    src/Danmaku.cpp \
    src/Graphic.cpp \
    src/Load.cpp \
    src/Local.cpp \
    src/Render.cpp \
    src/Shield.cpp \
    src/Utils.cpp

HEADERS  += \
    src/APlayer.h \
    src/Config.h \
    src/Danmaku.h \
    src/Graphic.h \
    src/Load.h \
    src/Local.h \
    src/Render.h \
    src/Shield.h \
    src/Utils.h

RESOURCES += \
    res/Res.qrc

TRANSLATIONS += \
    res/zh_CN.ts \
    res/zh_TW.ts

android{
DEFINES += BACKEND_QMM
DEFINES += RENDER_OPENGL
DEFINES += EMBEDDED
QT += multimedia
TEMPLATE = lib
}
else{
SOURCES += \
    src/Interface.cpp \
    src/Menu.cpp \
    src/Info.cpp \
    src/Editor.cpp \
    src/Post.cpp \
    src/Next.cpp \
    src/Search.cpp \
    src/Plugin.cpp

HEADERS += \
    src/Interface.h \
    src/Menu.h \
    src/Info.h \
    src/Editor.h \
    src/Post.h \
    src/Next.h \
    src/Search.h \
    src/Plugin.h

linux{
DEFINES += BACKEND_VLC BACKEND_QMM
DEFINES += RENDER_RASTER RENDER_OPENGL
QT += multimedia
}

win32{
RC_ICONS = BiliLocal.ico
DEFINES += BACKEND_VLC BACKEND_QMM
DEFINES += RENDER_RASTER RENDER_OPENGL
QT += multimedia
}

macx{
DEFINES += BACKEND_VLC
DEFINES += RENDER_RASTER RENDER_OPENGL
}
}

contains(DEFINES, BACKEND_VLC){
LIBS += \
    -lvlc \
    -lvlccore
}

contains(DEFINES, RENDER_RASTER){
LIBS += \
    -lswscale \
    -lavutil
}
