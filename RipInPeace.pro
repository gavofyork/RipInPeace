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
debug: DEFINES += DEBUG

SOURCES += main.cpp \
    RIP.cpp \
    Paranoia.cpp \
    Settings.cpp \
    DiscInfo.cpp

HEADERS  += \
    RIP.h \
    Paranoia.h \
    DiscInfo.h \
    Settings.h

FORMS += \
    Info.ui \
    Settings.ui

RESOURCES += \
    RIP.qrc

unix {
  #VARIABLES
  isEmpty(PREFIX) {
    PREFIX = /usr
  }
  BINDIR = $$PREFIX/bin
  DATADIR =$$PREFIX/share

  DEFINES += DATADIR=\\\"$$DATADIR\\\" PKGDATADIR=\\\"$$PKGDATADIR\\\"
  INSTALLS += target desktop service iconxpm icon26 icon48 icon64

  system (cat $${TARGET}.desktop > $${DESTDIR_TARGET}.desktop)
  system (echo "Exec=$$BINDIR/$${TARGET}" >> $${DESTDIR_TARGET}.desktop)
  system (echo "Icon=$$DATADIR/icons/hicolor/64x64/apps/$${TARGET}.png" >> $${DESTDIR_TARGET}.desktop)

  target.path = $$BINDIR
  desktop.path = $$DATADIR/applications
  desktop.files += $${TARGET}.desktop
  icon64.path = $$DATADIR/icons/hicolor/64x64/apps
  icon64.files += $${TARGET}.png
}
