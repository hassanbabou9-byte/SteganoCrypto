QT += core gui sql testlib
CONFIG += console testcase c++17
CONFIG -= app_bundle

TARGET = tst_core
TEMPLATE = app

include(../core/core.pri)

INCLUDEPATH += ../app
HEADERS += ../app/databasemanager.h
SOURCES += tst_core.cpp ../app/databasemanager.cpp
