QT += core gui widgets testlib
CONFIG += console testcase c++17
CONFIG -= app_bundle

TARGET = tst_gui
TEMPLATE = app

include(../core/core.pri)

INCLUDEPATH += ../app
HEADERS += ../app/mainwindow.h
SOURCES += tst_gui.cpp ../app/mainwindow.cpp
