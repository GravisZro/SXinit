TEMPLATE = app
CONFIG -= qt
CONFIG += c++11

QMAKE_CXXFLAGS += -std=c++14
QMAKE_CXXFLAGS += -pipe -Os -fno-exceptions -fno-rtti -fno-threadsafe-statics
#QMAKE_CXXFLAGS += -pipe -Os
#QMAKE_CXXFLAGS += -fno-exceptions
#QMAKE_CXXFLAGS += -fno-rtti
#QMAKE_CXXFLAGS += -fno-threadsafe-statics

LIBS += -lpthread

INCLUDEPATH += ../pdtk

SOURCES += \
    main.cpp \
    ../pdtk/application.cpp \
    ../pdtk/process.cpp \
    ../pdtk/fsentry.cpp

HEADERS += \
    ../pdtk/application.h \
    ../pdtk/object.h \
    ../pdtk/process.h \
    ../pdtk/fsentry.h \
    ../pdtk/cxxutils/posix_helpers.h \
    ../pdtk/cxxutils/error_helpers.h
