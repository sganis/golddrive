#-------------------------------------------------
#
# Project created by QtCreator 2018-08-31T12:09:48
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ssh-drive
TEMPLATE = app
CONFIG += no_keywords
DEFINES += QT_DEPRECATED_WARNINGS
DESTDIR = $$PWD/release/app

INCLUDEPATH += C:\Python37\include
LIBS += -LC:\Python37\libs -lpython37

SOURCES += \
        main.cpp \
        mainwindow.cpp

HEADERS += \
        mainwindow.h

FORMS += \
        mainwindow.ui
