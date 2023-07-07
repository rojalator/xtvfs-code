#-------------------------------------------------
#
# Project created by QtCreator 2013-12-05T09:43:56
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = xtvfsreader
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    filesystem.cpp

HEADERS  += mainwindow.h \
    filesystem.h

FORMS    += mainwindow.ui
