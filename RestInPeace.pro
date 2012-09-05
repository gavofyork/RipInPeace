#-------------------------------------------------
#
# Project created by QtCreator 2012-09-03T13:21:44
#
#-------------------------------------------------

QT       += core gui script

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = RipInPeace
TEMPLATE = app

QMAKE_CXXFLAGS += --std=c++0x
LIBS += -lcdio_paranoia -lcdio_cdda -lcdio -lcddb -lFLAC++ -ldiscid -lmusicbrainz4

DEFINES += HAVE_LIBCDIO

SOURCES += main.cpp \
    RIP.cpp \
    Paranoia.cpp

HEADERS  += \
    RIP.h \
    Paranoia.h \
    DiscInfo.h

FORMS += \
    Info.ui

RESOURCES += \
    RIP.qrc

