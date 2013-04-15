QT += \
    core \
    gui \
    widgets \
    network

TARGET = BiliLocal

TEMPLATE = app

CONFIG += c++11

SOURCES += \
    Interface.cpp \
    Local.cpp \
    Danmaku.cpp \
    VPlayer.cpp \
    Menu.cpp \
    State.cpp

HEADERS  += \
    Interface.h \
    Danmaku.h \
    VPlayer.h \
    Menu.h \
    State.h

LIBS += -L./\
    -lvlc \
    -lvlccore \
    -lavutil \
    -lswscale

RESOURCES += \
    Res.qrc

TRANSLATIONS = \
    zh_CN.ts

RC_FILE = \
    Icon.rc
