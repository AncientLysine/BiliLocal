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
    src/List.cpp \
    src/Load.cpp \
    src/Local.cpp \
    src/Render.cpp \
    src/Shield.cpp \
    src/Utils.cpp \
    src/Interface.cpp \
    src/Menu.cpp \
    src/Info.cpp \
    src/Editor.cpp \
    src/Post.cpp \
    src/Jump.cpp \
    src/Search.cpp \
    src/Plugin.cpp

HEADERS  += \
    src/APlayer.h \
    src/Config.h \
    src/Danmaku.h \
    src/Graphic.h \
    src/List.h \
    src/Load.h \
    src/Local.h \
    src/Render.h \
    src/Shield.h \
    src/Utils.h \
    src/Interface.h \
    src/Menu.h \
    src/Info.h \
    src/Editor.h \
    src/Post.h \
    src/Jump.h \
    src/Search.h \
    src/Plugin.h

RESOURCES += \
    res/Res.qrc

TRANSLATIONS += \
    res/zh_CN.ts \
    res/zh_TW.ts

linux{
DEFINES += BACKEND_VLC BACKEND_QMM
DEFINES += RENDER_RASTER RENDER_OPENGL
}

win32{
RC_ICONS = BiliLocal.ico
DEFINES += BACKEND_VLC BACKEND_QMM
DEFINES += RENDER_RASTER RENDER_OPENGL
}

macx{
DEFINES += BACKEND_VLC
DEFINES += RENDER_OPENGL
}

contains(DEFINES, BACKEND_QMM){
QT += \
    multimedia
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
