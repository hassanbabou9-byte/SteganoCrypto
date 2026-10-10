QT += core gui widgets sql
CONFIG += c++17

TARGET = SteganoCrypto
TEMPLATE = app

include(../core/core.pri)

HEADERS += \
    databasemanager.h \
    loginwindow.h \
    mainwindow.h \
    signupwindow.h

SOURCES += \
    databasemanager.cpp \
    loginwindow.cpp \
    main.cpp \
    mainwindow.cpp \
    signupwindow.cpp
