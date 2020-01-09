TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle

#CONFIG -= qt  # test with qt.
CONFIG -= console

#VLIBS_DIR = $$PWD/../..
#include( $$VLIBS_DIR/qmake/gtest.pri )
#include( $$VLIBS_DIR/qmake/vlog.pri  )

#INCLUDEPATH += $$PWD

SOURCES += main.c

CONFIG      *= link_pkgconfig
PKGCONFIG   *= apr-1

include( $$PWD/../../sdk_core/sdk_core.pri )
