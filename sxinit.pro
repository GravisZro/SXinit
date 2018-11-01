TEMPLATE = app
CONFIG -= qt
CONFIG -= c++11
CONFIG -= c++14

# FOR CLANG
#QMAKE_CXXFLAGS += -stdlib=libc++
#QMAKE_LFLAGS += -stdlib=libc++
#QMAKE_CXXFLAGS += -fsanitize=address
#QMAKE_CXXFLAGS += -fsanitize=bounds

# universal arguments
QMAKE_CXXFLAGS += -std=c++14
QMAKE_CXXFLAGS += -pipe -Os
QMAKE_CXXFLAGS += -fno-exceptions
QMAKE_CXXFLAGS += -fno-rtti
QMAKE_CXXFLAGS += -fno-threadsafe-statics
QMAKE_CXXFLAGS += -fno-asynchronous-unwind-tables
#QMAKE_CXXFLAGS += -fstack-protector-all
QMAKE_CXXFLAGS += -fstack-protector-strong

# optimizations
QMAKE_CXXFLAGS += -fdata-sections
QMAKE_CXXFLAGS += -ffunction-sections
QMAKE_LFLAGS += -Wl,--gc-sections

# defines
DEFINES += INTERRUPTED_WRAPPER

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
    $$PUT/specialized/procstat.cpp \
    $$PUT/specialized/eventbackend.cpp \
    $$PUT/specialized/peercred.cpp \
    $$PUT/specialized/proclist.cpp \
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
    $$PUT/specialized/procstat.h \
    $$PUT/specialized/eventbackend.h \
    $$PUT/specialized/mount.h \
    $$PUT/specialized/blockdevices.h \
    $$PUT/specialized/peercred.h \
    $$PUT/specialized/proclist.h \
    $$PUT/specialized/module.h \
    $$PUT/specialized/fstable.h \
    $$PUT/specialized/MountEvent.h \
    $$PUT/specialized/FileEvent.h \
    $$PUT/specialized/PollEvent.h \
    $$PUT/specialized/ProcessEvent.h
