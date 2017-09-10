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
#DEFINES += GLOBAL_PROCESS_EVENT_TRACKING
#DEFINES += ENABLE_UEVENT_TRACKING
DEFINES += INTERRUPTED_WRAPPER
DEFINES += WANT_CONFIG_SERVICE
DEFINES += WANT_PROCFS
#LIBS += -lpthread

PDTK = ../pdtk
INCLUDEPATH += $$PDTK

SOURCES += \
    main.cpp \
    framebuffer.cpp \
    fstable.cpp \
    initializer.cpp \
    display.cpp \
    $$PDTK/application.cpp \
    $$PDTK/process.cpp \
    $$PDTK/socket.cpp \
    $$PDTK/cxxutils/configmanip.cpp \
    $$PDTK/specialized/procstat.cpp \
    $$PDTK/specialized/eventbackend.cpp \
    $$PDTK/specialized/peercred.cpp \
    $$PDTK/specialized/proclist.cpp \
    $$PDTK/specialized/mount.cpp

HEADERS += \
    framebuffer.h \
    fstable.h \
    initializer.h \
    splash.h \
    display.h \
    $$PDTK/application.h \
    $$PDTK/object.h \
    $$PDTK/process.h \
    $$PDTK/cxxutils/vfifo.h \
    $$PDTK/cxxutils/posix_helpers.h \
    $$PDTK/cxxutils/error_helpers.h \
    $$PDTK/cxxutils/cstringarray.h \
    $$PDTK/cxxutils/hashing.h \
    $$PDTK/cxxutils/vterm.h \
    $$PDTK/cxxutils/pipedspawn.h \
    $$PDTK/cxxutils/sharedmem.h \
    $$PDTK/cxxutils/configmanip.h \
    $$PDTK/cxxutils/syslogstream.h \
    $$PDTK/cxxutils/socket_helpers.h \
    $$PDTK/specialized/procstat.h \
    $$PDTK/specialized/eventbackend.h \
    $$PDTK/specialized/mount.h \
    $$PDTK/specialized/peercred.h \
    $$PDTK/specialized/proclist.h \
    $$PDTK/socket.h
