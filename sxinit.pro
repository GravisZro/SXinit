TEMPLATE = app
CONFIG -= qt
#CONFIG += c++14

# FOR CLANG
#QMAKE_CXXFLAGS += -stdlib=libc++
#QMAKE_LFLAGS += -stdlib=libc++

# universal arguments
QMAKE_CXXFLAGS += -std=c++14
QMAKE_CXXFLAGS += -pipe -Os -fno-exceptions -fno-rtti -fno-threadsafe-statics
#QMAKE_CXXFLAGS += -pipe -Os
#QMAKE_CXXFLAGS += -fno-exceptions
#QMAKE_CXXFLAGS += -fno-rtti
#QMAKE_CXXFLAGS += -fno-threadsafe-statics
DEFINES += ENABLE_PROCESS_EVENT_TRACKING
#DEFINES += ENABLE_UEVENT_TRACKING
DEFINES += INTERRUPTED_WRAPPER
#LIBS += -lpthread

INCLUDEPATH += ../pdtk

SOURCES += \
    main.cpp \
    ../pdtk/application.cpp \
    ../pdtk/process.cpp \
    ../pdtk/specialized/procstat.cpp \
    ../pdtk/specialized/eventbackend.cpp \
    ../pdtk/cxxutils/configmanip.cpp \
    ../pdtk/specialized/peercred.cpp \
    ../pdtk/socket.cpp \
    ../pdtk/specialized/proclist.cpp \
    framebuffer.cpp \
    fstable.cpp \
    ../pdtk/specialized/mount.cpp \
    initializer.cpp

HEADERS += \
    ../pdtk/application.h \
    ../pdtk/object.h \
    ../pdtk/process.h \
    ../pdtk/cxxutils/vfifo.h \
    ../pdtk/cxxutils/posix_helpers.h \
    ../pdtk/cxxutils/error_helpers.h \
    ../pdtk/specialized/procstat.h \
    ../pdtk/specialized/eventbackend.h \
    ../pdtk/cxxutils/cstringarray.h \
    ../pdtk/cxxutils/pipedspawn.h \
    ../pdtk/cxxutils/sharedmem.h \
    ../pdtk/cxxutils/configmanip.h \
    ../pdtk/cxxutils/syslogstream.h \
    ../pdtk/cxxutils/socket_helpers.h \
    ../pdtk/specialized/peercred.h \
    ../pdtk/cxxutils/colors.h \
    ../pdtk/socket.h \
    ../pdtk/specialized/proclist.h \
    framebuffer.h \
    fstable.h \
    ../pdtk/specialized/mount.h \
    initializer.h \
    splash.h
