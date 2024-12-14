#-------------------------------------------------
#
# Project created by QtCreator 2024-10-22T13:19:36
#
#-------------------------------------------------

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
LIBS += -lusb-1.0

TARGET = SFP-master
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    chunks.cpp \
    qhexedit.cpp \
    commands.cpp \
    ch341funcs.c \
    dialogabout.cpp \
    dialogpass.cpp

HEADERS += \
        mainwindow.h \
    chunks.h \
    mainwindow.h \
    commands.h \
    qhexedit.h \
    ch341eeprom.h \
    dialogabout.h \
    dialogpass.h

FORMS += \
        mainwindow.ui \
    dialogabout.ui \
    dialogpass.ui

TRANSLATIONS += language/SFP-Master_hu_HU.ts \
         language/SFP-Master_ru_RU.ts \
         language/SFP-Master_it_IT.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc
