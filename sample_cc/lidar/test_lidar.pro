TEMPLATE = app

CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= console

SOURCES += main.cpp lds_lidar.cpp livoxthread.cpp

HEADERS += lds_lidar.h livoxthread.h

include( $$PWD/../../sdk_core/sdk_core.pri )
include( $$PWD/../../plot/plot.pri )
