TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle

CONFIG -= console

SOURCES += main.cpp

include( $$PWD/../../sdk_core/sdk_core.pri )

INCLUDEPATH += $$PWD/../../sdk_core/src
