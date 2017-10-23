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
DEFINES += INTERRUPTED_WRAPPER

linux:DEFINES += WANT_MOUNT_ROOT
linux:DEFINES += WANT_MODULES
linux:DEFINES += WANT_SYSFS

DEFINES += WANT_CONFIG_SERVICE
DEFINES += WANT_MCFS

#LIBS += -lpthread
experimental {
#QMAKE_CXXFLAGS += -stdlib=libc++
QMAKE_CXXFLAGS += -nostdinc
INCLUDEPATH += /usr/include/x86_64-linux-musl
INCLUDEPATH += /usr/include/c++/v1
INCLUDEPATH += /usr/include
INCLUDEPATH += /usr/include/x86_64-linux-gnu
QMAKE_LFLAGS += -L/usr/lib/x86_64-linux-musl -dynamic-linker /lib/ld-musl-x86_64.so.1
LIBS += -lc++
}

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
    $$PDTK/specialized/mount.cpp \
    $$PDTK/specialized/blockdevices.cpp \
    $$PDTK/specialized/module.cpp \
    $$PDTK/specialized/FileEvent.cpp \
    $$PDTK/specialized/PollEvent.cpp \
    $$PDTK/specialized/ProcessEvent.cpp

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
    $$PDTK/specialized/blockdevices.h \
    $$PDTK/specialized/peercred.h \
    $$PDTK/specialized/proclist.h \
    $$PDTK/socket.h \
    $$PDTK/specialized/module.h \
    $$PDTK/specialized/FileEvent.h \
    $$PDTK/specialized/PollEvent.h \
    $$PDTK/specialized/ProcessEvent.h
