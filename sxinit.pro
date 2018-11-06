TEMPLATE = app
CONFIG -= qt
CONFIG += c++14
CONFIG += strict_c++
CONFIG += exceptions_off
CONFIG += rtti_off

# FOR CLANG
#QMAKE_CXXFLAGS += -stdlib=libc++
#QMAKE_LFLAGS += -stdlib=libc++
QMAKE_CXXFLAGS += -fconstexpr-depth=256
QMAKE_CXXFLAGS += -fconstexpr-steps=900000000

# universal arguments
QMAKE_CXXFLAGS += -fno-rtti

QMAKE_CXXFLAGS_DEBUG += -O0 -g3
QMAKE_CXXFLAGS_RELEASE += -Os


#QMAKE_CXXFLAGS_RELEASE += -fno-threadsafe-statics
QMAKE_CXXFLAGS_RELEASE += -fno-asynchronous-unwind-tables
#QMAKE_CXXFLAGS_RELEASE += -fstack-protector-all
QMAKE_CXXFLAGS_RELEASE += -fstack-protector-strong

# optimizations
QMAKE_CXXFLAGS_RELEASE += -fdata-sections
QMAKE_CXXFLAGS_RELEASE += -ffunction-sections
QMAKE_LFLAGS_RELEASE += -Wl,--gc-sections

# libraries
LIBS += -lrt

# defines
#DEFINES += INTERRUPTED_WRAPPER

DEFINES += WANT_MOUNT_ROOT
DEFINES += WANT_MODULES
linux:DEFINES += WANT_SYSFS
DEFINES += WANT_NATIVE_SCFS

DEFINES += WANT_CONFIG_SERVICE
DEFINES += WANT_FUSE_SCFS

#LIBS += -lpthread
experimental {
INCLUDEPATH += /usr/include/x86_64-linux-musl
INCLUDEPATH += /usr/include/c++/v1
INCLUDEPATH += /usr/include
INCLUDEPATH += /usr/include/x86_64-linux-gnu
QMAKE_LFLAGS += -L/usr/lib/x86_64-linux-musl -dynamic-linker /lib/ld-musl-x86_64.so.1
LIBS += -lc++
}

PUT = ../put
INCLUDEPATH += $$PUT

SOURCES += \
    main.cpp \
    framebuffer.cpp \
    initializer.cpp \
    display.cpp \
    $$PUT/application.cpp \
    $$PUT/childprocess.cpp \
    $$PUT/socket.cpp \
    $$PUT/asyncfd.cpp \
    $$PUT/cxxutils/vfifo.cpp \
    $$PUT/cxxutils/configmanip.cpp \
    $$PUT/cxxutils/syslogstream.cpp \
    $$PUT/cxxutils/mountpoint_helpers.cpp \
    $$PUT/specialized/eventbackend.cpp \
    $$PUT/specialized/mutex.cpp \
    $$PUT/specialized/procstat.cpp \
    $$PUT/specialized/proclist.cpp \
    $$PUT/specialized/peercred.cpp \
    $$PUT/specialized/mount.cpp \
    $$PUT/specialized/blockdevices.cpp \
    $$PUT/specialized/module.cpp \
    $$PUT/specialized/fstable.cpp \
    $$PUT/specialized/MountEvent.cpp \
    $$PUT/specialized/FileEvent.cpp \
    $$PUT/specialized/PollEvent.cpp \
    $$PUT/specialized/ProcessEvent.cpp

HEADERS += \
    framebuffer.h \
    initializer.h \
    splash.h \
    display.h \
    $$PUT/object.h \
    $$PUT/application.h \
    $$PUT/childprocess.h \
    $$PUT/socket.h \
    $$PUT/asyncfd.h \
    $$PUT/cxxutils/vfifo.h \
    $$PUT/cxxutils/configmanip.h \
    $$PUT/cxxutils/syslogstream.h \
    $$PUT/cxxutils/posix_helpers.h \
    $$PUT/cxxutils/error_helpers.h \
    $$PUT/cxxutils/cstringarray.h \
    $$PUT/cxxutils/hashing.h \
    $$PUT/cxxutils/vterm.h \
    $$PUT/cxxutils/pipedspawn.h \
    $$PUT/cxxutils/socket_helpers.h \
    $$PUT/cxxutils/mountpoint_helpers.h \
    $$PUT/specialized/osdetect.h \
    $$PUT/specialized/eventbackend.h \
    $$PUT/specialized/mutex.h \
    $$PUT/specialized/procstat.h \
    $$PUT/specialized/proclist.h \
    $$PUT/specialized/peercred.h \
    $$PUT/specialized/mount.h \
    $$PUT/specialized/blockdevices.h \
    $$PUT/specialized/module.h \
    $$PUT/specialized/fstable.h \
    $$PUT/specialized/MountEvent.h \
    $$PUT/specialized/FileEvent.h \
    $$PUT/specialized/PollEvent.h \
    $$PUT/specialized/ProcessEvent.h
