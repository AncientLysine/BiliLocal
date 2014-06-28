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
    src/APlayer.cpp \
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
    src/APlayer.h \
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

RESOURCES += \
    res/Res.qrc

TRANSLATIONS += \
    res/zh_CN.ts \
    res/zh_TW.ts

android{
DEFINES += BACKEND_QMM
DEFINES += RENDER_OPENGL
QT += multimedia
ANDROID_PACKAGE_SOURCE_DIR = $$PWD/res
OTHER_FILES += res/AndroidManifest.xml

}
else{
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
