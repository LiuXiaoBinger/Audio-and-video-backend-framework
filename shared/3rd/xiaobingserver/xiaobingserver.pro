TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += c++11



LIBS += -lpthread
LIBS += -lmysqlclient

SOURCES += main.cpp \
    thread_pool.cpp \
    fileserver.cpp \
    mysql.cpp \
    ckenel.cpp \
    Thread_pool.cpp \
    err_str.cpp

HEADERS += \
    thread_pool.h \
    fileserver.h \
    mysql.h \
    packdef.h \
    ckenel.h \
    Thread_pool.h \
    err_str.h

