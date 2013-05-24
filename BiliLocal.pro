QT += \
    core \
    gui \
    widgets \
    network \
    sql

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
    src/Poster.cpp

HEADERS  += \
    src/Interface.h \
    src/Danmaku.h \
    src/VPlayer.h \
    src/Menu.h \
    src/Info.h \
    src/Search.h \
    src/Utils.h \
    src/Shield.h \
    src/Poster.h

LIBS += -L./\
    -lvlc \
    -lvlccore \
    -lavutil \
    -lswscale

INCLUDEPATH += include/

RESOURCES += \
    res/Res.qrc

TRANSLATIONS += \
    trans/zh_CN.ts \
    trans/zh_HK.ts \
    trans/zh_TW.ts

win32 {
RC_FILE = \
    Icon.rc
}
macx {
    QMAKE_CXXFLAGS = -mmacosx-version-min=10.7 -std=gnu0x -stdlib=libc++
}
