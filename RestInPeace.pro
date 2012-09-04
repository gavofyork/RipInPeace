#-------------------------------------------------
#
# Project created by QtCreator 2012-09-03T13:21:44
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = RestInPeace
TEMPLATE = app

QMAKE_CXXFLAGS += --std=c++0x
LIBS += -lcdio_paranoia -lcdio_cdda -lcdio -lcddb -lFLAC++ -ldiscid -lmusicbrainz4

SOURCES += main.cpp \
    RIP.cpp \
    Paranoia.cpp

HEADERS  += \
    RIP.h \
    Paranoia.h \
    DiscInfo.h

FORMS += \
    Info.ui

