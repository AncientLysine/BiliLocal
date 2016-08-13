QT += \
    core \
    gui \
    network \
    concurrent

TARGET = BiliLocal

TEMPLATE = app

CONFIG += c++11

SOURCES += \
    src/Access/Load.cpp \
    src/Access/NetworkConfiguration.cpp \
    src/Access/Parse.cpp \
    src/Access/Post.cpp \
    src/Access/Seek.cpp \
    src/Access/Sign.cpp \
    src/Graphic/Graphic.cpp \
    src/Graphic/GraphicPrivate.cpp \
    src/Graphic/Mode1.cpp \
    src/Graphic/Mode4.cpp \
    src/Graphic/Mode5.cpp \
    src/Graphic/Mode6.cpp \
    src/Graphic/Mode7.cpp \
    src/Graphic/Plain.cpp \
    src/Model/Danmaku.cpp \
    src/Model/Running.cpp \
    src/Model/List.cpp \
    src/Model/Shield.cpp \
    src/Player/APlayer.cpp \
    src/Render/ARender.cpp \
    src/Render/ASprite.cpp \
    src/UI/Interface.cpp \
    src/Bundle.cpp \
    src/Config.cpp \
    src/Local.cpp \
    src/Plugin.cpp \
    src/Utils.cpp

HEADERS += \
    src/Access/AccessPrivate.h \
    src/Access/Load.h \
    src/Access/NetworkConfiguration.h \
    src/Access/Parse.h \
    src/Access/Post.h \
    src/Access/Seek.h \
    src/Access/Sign.h \
    src/Graphic/Graphic.h \
    src/Graphic/GraphicPrivate.h \
    src/Graphic/Mode1.h \
    src/Graphic/Mode4.h \
    src/Graphic/Mode5.h \
    src/Graphic/Mode6.h \
    src/Graphic/Mode7.h \
    src/Graphic/Plain.h \
    src/Model/Danmaku.h \
    src/Model/Running.h \
    src/Model/List.h \
    src/Model/Shield.h \
    src/Player/APlayer.h \
    src/Render/ARender.h \
    src/Render/ARenderPrivate.h \
    src/Render/ASprite.h \
    src/Render/ElapsedTimer.h \
    src/Render/PFormat.h \
    src/UI/Interface.h \
    src/UI/InterfacePrivate.h \
    src/Bundle.h \
    src/Config.h \
    src/Local.h \
    src/Plugin.h \
    src/Utils.h

INCLUDEPATH += \
    src

PRECOMPILED_HEADER = \
    src/Common.h

RESOURCES += \
    res/Res.qrc

TRANSLATIONS += \
    res/zh_CN.ts \
    res/zh_TW.ts

CONFIG(debug, debug|release){
DEFINES += GRAPHIC_DEBUG
}

linux : !android{
DEFINES += \
    BACKEND_VLC \
    BACKEND_QMM \
    BACKEND_NIL

DEFINES += \
    RENDER_RASTER \
    RENDER_OPENGL

DEFINES += \
    INTERFACE_WIDGET

LIBS += \
    -lcrypto \
    -lssl
}

win32{
RC_ICONS = res\icon.ico

DEFINES += \
    BACKEND_VLC \
    BACKEND_QMM \
    BACKEND_NIL

DEFINES += \
    RENDER_OPENGL

DEFINES += \
    INTERFACE_WIDGET

INCLUDEPATH += \
    D:/App/Programming/include

!contains(QMAKE_TARGET.arch, x86_64){
DEPENDPATH += \
    -LD:/App/Programming/lib
}
else{
DEPENDPATH += \
    -LD:/App/Programming/lib/amd64
}

LIBS += \
    -llibeay32 \
    -lssleay32
}

macx{
DEFINES += \
    BACKEND_VLC \
    BACKEND_NIL

DEFINES += \
    RENDER_OPENGL

DEFINES += \
    INTERFACE_WIDGET

LIBS += \
    -lcrypto \
    -lssl
}

android{
DEFINES += \
    BACKEND_QMM \
    BACKEND_NIL

DEFINES += \
    RENDER_OPENGL

DEFINES += \
    INTERFACE_QUICK2

DISTFILES += \
    res/Android/AndroidManifest.xml \
    res/Android/gradle/wrapper/gradle-wrapper.jar \
    res/Android/gradlew \
    res/Android/res/values/libs.xml \
    res/Android/build.gradle \
    res/Android/gradle/wrapper/gradle-wrapper.properties \
    res/Android/gradlew.bat

RESOURCES += \
    src/UI/Quick2/BundleQuick2.qrc

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/res/Android
}

contains(DEFINES, INTERFACE_WIDGET){
QT += widgets

SOURCES += \
    src/UI/Widget/Editor.cpp \
    src/UI/Widget/Home.cpp \
    src/UI/Widget/Info.cpp \
    src/UI/Widget/Jump.cpp \
    src/UI/Widget/Menu.cpp \
    src/UI/Widget/Prefer.cpp \
    src/UI/Widget/Search.cpp \
    src/UI/Widget/Type.cpp \
    src/UI/Widget/WidgetInterfacePrivate.cpp

HEADERS += \
    src/UI/Widget/Editor.h \
    src/UI/Widget/Home.h \
    src/UI/Widget/Info.h \
    src/UI/Widget/Jump.h \
    src/UI/Widget/Menu.h \
    src/UI/Widget/Prefer.h \
    src/UI/Widget/Search.h \
    src/UI/Widget/Type.h \
    src/UI/Widget/WidgetInterfacePrivate.h

message(enable widget interface)
}

contains(DEFINES, INTERFACE_QUICK2){
QT+= \
    qml \
    quick

lupdate_only{
SOURCES += \
    src/UI/Quick2/Home.qml \
    src/UI/Quick2/Info.qml \
    src/UI/Quick2/Interface.qml \
    src/UI/Quick2/Menu.qml
}

SOURCES += \
    src/UI/Quick2/Quick2InterfacePrivate.cpp

HEADERS += \
    src/UI/Quick2/Export.h \
    src/UI/Quick2/Quick2InterfacePrivate.h

message(enable quick2 interface)
}

win32 : contains(QT, widgets) : !contains(QMAKE_TARGET.arch, x86_64){
DEFINES += \
    RENDER_RASTER
}

contains(DEFINES, RENDER_RASTER){
SOURCES += \
    src/Render/Raster/RasterRender.cpp \
    src/Render/Raster/AsyncRasterSprite.cpp

HEADERS += \
    src/Render/Raster/RasterRender.h \
    src/Render/Raster/RasterRenderPrivate.h \
    src/Render/Raster/AsyncRasterSprite.h

LIBS += \
    -lswscale \
    -lavutil

message(enable raster render widget output)
}

contains(DEFINES, RENDER_OPENGL){
SOURCES += \
    src/Render/OpenGL/OpenGLRender.cpp \
    src/Render/OpenGL/OpenGLAtlas.cpp \
    src/Render/OpenGL/SyncTextureSprite.cpp \
    src/Render/OpenGL/DetachPrivate.cpp \
    src/Render/OpenGL/OpaquePrivate.cpp
HEADERS += \
    src/Render/OpenGL/OpenGLRender.h \
    src/Render/OpenGL/OpenGLRenderPrivate.h \
    src/Render/OpenGL/OpenGLAtlas.h \
    src/Render/OpenGL/SyncTextureSprite.h \
    src/Render/OpenGL/DetachPrivate.h \
    src/Render/OpenGL/OpaquePrivate.h

message(enable opengl render detach output)

contains(QT, widgets){
HEADERS += \
    src/Render/OpenGL/WidgetPrivate.h \
    src/Render/OpenGL/WindowPrivate.h

SOURCES += \
    src/Render/OpenGL/WidgetPrivate.cpp \
    src/Render/OpenGL/WindowPrivate.cpp

message(enable opengl render widget output)
message(enable opengl render window output)
}

contains(QT, quick){
HEADERS += \
    src/Render/OpenGL/Quick2Private.h

SOURCES += \
    src/Render/OpenGL/Quick2Private.cpp
}

message(enable opengl render quick2 output)
}

contains(DEFINES, BACKEND_VLC){
SOURCES += \
    src/Player/VPlayer.cpp

HEADERS += \
    src/Player/VPlayer.h

LIBS += \
    -lvlc \
    -lvlccore
message(enable vplayer libvlc backend)
}

contains(DEFINES, BACKEND_QMM){
SOURCES += \
    src/Player/QPlayer.cpp

HEADERS += \
    src/Player/QPlayer.h

QT += \
    multimedia

message(enable qplayer libqtmultimedia backend)
}

contains(DEFINES, BACKEND_NIL){
SOURCES += \
    src/Player/NPlayer.cpp

HEADERS += \
    src/Player/NPlayer.h

message(enable nplayer dummy backend)
}
