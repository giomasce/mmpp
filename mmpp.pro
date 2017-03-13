
CONFIG -= qt

CONFIG(qt) {
    QT       += core gui
    greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
    DEFINES += USE_QT
}

TEMPLATE = app
CONFIG += link_pkgconfig
PKGCONFIG += jsoncpp libmicrohttpd

SOURCES += main.cpp \
    parser.cpp \
    library.cpp \
    proof.cpp \
    unification.cpp \
    memory.c \
    wff.cpp \
    toolbox.cpp \
    earley.cpp \
    test.cpp \
    utils.cpp \
    httpd.cpp \
    web.cpp

CONFIG(qt) {
SOURCES +=  mainwindow.cpp \
    prooftreemodel.cpp \
    htmldelegate.cpp
}

QMAKE_CXXFLAGS += -std=c++17 -march=native
QMAKE_LIBS += -ldl -export-dynamic -rdynamic -lboost_system -lboost_filesystem -lpthread

HEADERS += \
    wff.h \
    parser.h \
    library.h \
    proof.h \
    unification.h \
    memory.h \
    toolbox.h \
    earley.h \
    stringcache.h \
    test.h \
    utils.h \
    httpd.h \
    web.h

CONFIG(qt) {
HEADERS +=  mainwindow.h \
    prooftreemodel.h \
    htmldelegate.h
}

DISTFILES += \
    README

FORMS += \
    mainwindow.ui
