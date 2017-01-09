#-------------------------------------------------
#
# Project created by QtCreator 2016-12-30T14:43:17
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Autonat
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    ownparameterdialog.cpp \
    backupstarter.cpp

HEADERS  += mainwindow.h \
    ownparameterdialog.h \
    backupstarter.h

FORMS    += mainwindow.ui \
    ownparameterdialog.ui

RESOURCES += \
    icons.qrc
