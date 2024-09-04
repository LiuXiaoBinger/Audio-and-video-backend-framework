TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -lpthread -lmysqlclient

INCLUDEPATH +=./include
HEADERS += \
    include/Thread_pool.h \
    include/TCPKernel.h \
    include/packdef.h \
    include/Mysql.h \
    include/err_str.h \
    include/block_epoll_net.h

OTHER_FILES +=

SOURCES += \
    src/Thread_pool.cpp \
    src/TCPKernel.cpp \
    src/Mysql.cpp \
    src/main.cpp \
    src/err_str.cpp \
    src/block_epoll_net.cpp



