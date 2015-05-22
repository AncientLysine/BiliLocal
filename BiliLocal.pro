QT += \
    core \
    gui \
    network \
    widgets

TARGET = BiliLocal

TEMPLATE = app

CONFIG += c++11

SOURCES += \
    src/Access/Load.cpp \
    src/Access/NetworkConfiguration.cpp \
    src/Access/Post.cpp \
    src/Graphic/Graphic.cpp \
    src/Graphic/GraphicPrivate.cpp \
    src/Graphic/Mode1.cpp \
    src/Graphic/Mode4.cpp \
    src/Graphic/Mode5.cpp \
    src/Graphic/Mode6.cpp \
    src/Graphic/Mode7.cpp \
    src/Graphic/Plain.cpp \
    src/Model/Danmaku.cpp \
    src/Model/List.cpp \
    src/Model/Shield.cpp \
    src/Player/APlayer.cpp \
    src/Render/ARender.cpp \
    src/UI/Editor.cpp \
    src/UI/Info.cpp \
    src/UI/Interface.cpp \
    src/UI/Jump.cpp \
    src/UI/Menu.cpp \
    src/UI/Prefer.cpp \
    src/UI/Search.cpp \
    src/UI/Type.cpp \
    src/Config.cpp \
    src/Local.cpp \
    src/Utils.cpp \
    src/Plugin.cpp

HEADERS += \
    src/Access/AccessPrivate.h \
    src/Access/Load.h \
    src/Access/NetworkConfiguration.h \
    src/Access/Post.h \
    src/Graphic/Graphic.h \
    src/Graphic/GraphicPrivate.h \
    src/Graphic/Mode1.h \
    src/Graphic/Mode4.h \
    src/Graphic/Mode5.h \
    src/Graphic/Mode6.h \
    src/Graphic/Mode7.h \
    src/Graphic/Plain.h \
    src/Model/Danmaku.h \
    src/Model/List.h \
    src/Model/Shield.h \
    src/Player/APlayer.h \
    src/Render/ARender.h \
    src/Render/ARenderPrivate.h \
    src/UI/Editor.h \
    src/UI/Info.h \
    src/UI/Interface.h \
    src/UI/Jump.h \
    src/UI/Menu.h \
    src/UI/Prefer.h \
    src/UI/Search.h \
    src/UI/Type.h \
    src/Config.h \
    src/Local.h \
    src/Utils.h \
    src/Plugin.h

RESOURCES += \
    res/Res.qrc

TRANSLATIONS += \
    res/zh_CN.ts \
    res/zh_TW.ts

linux{
DEFINES += \
    BACKEND_VLC

DEFINES += \
    RENDER_RASTER \
    RENDER_OPENGL
}

win32{
RC_ICONS = BiliLocal.ico

DEFINES += \
    BACKEND_VLC \
    BACKEND_QMM \
    BACKEND_NIL

DEFINES += \
    RENDER_RASTER \
    RENDER_OPENGL \
    RENDER_DETACH
}

macx{
DEFINES += \
    BACKEND_VLC

DEFINES += \
    RENDER_OPENGL
}

contains(DEFINES, BACKEND_VLC){
SOURCES += \
    src/Player/VPlayer.cpp

HEADERS += \
    src/Player/VPlayer.h

LIBS += \
    -lvlc \
    -lvlccore
}

contains(DEFINES, BACKEND_QMM){
SOURCES += \
    src/Player/QPlayer.cpp

HEADERS += \
    src/Player/QPlayer.h

QT += \
    multimedia
}

contains(DEFINES, BACKEND_NIL){
SOURCES += \
    src/Player/NPlayer.cpp

HEADERS += \
    src/Player/NPlayer.h

QT += \
    multimedia
}

contains(DEFINES, RENDER_RASTER){
SOURCES += \
    src/Render/RasterRender.cpp

HEADERS += \
    src/Render/RasterRender.h \
    src/Render/AsyncRasterCache.h

LIBS += \
    -lswscale \
    -lavutil
}

contains(DEFINES, RENDER_OPENGL){
SOURCES += \
    src/Render/OpenGLRender.cpp \
    src/Render/OpenGLRenderPrivateBase.cpp

HEADERS += \
    src/Render/OpenGLRender.h \
    src/Render/OpenGLRenderPrivateBase.h \
    src/Render/SyncTextureCache.h
}

contains(DEFINES, RENDER_DETACH){
SOURCES += \
    src/Render/DetachRender.cpp

HEADERS += \
    src/Render/DetachRender.h
}
