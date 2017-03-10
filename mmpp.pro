
QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TEMPLATE = app
CONFIG += link_pkgconfig
PKGCONFIG += jsoncpp

SOURCES += main.cpp \
    parser.cpp \
    library.cpp \
    proof.cpp \
    unification.cpp \
    memory.c \
    wff.cpp \
    toolbox.cpp \
    earley.cpp \
    mainwindow.cpp \
    test.cpp \
    utils.cpp \
    prooftreemodel.cpp \
    htmldelegate.cpp \
    httpd.cpp \
    webendpoint.cpp

QMAKE_CXXFLAGS += -std=c++17 -march=native
QMAKE_LIBS += -ldl -export-dynamic -rdynamic -lboost_system -lboost_filesystem -lmicrohttpd

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
    mainwindow.h \
    test.h \
    utils.h \
    prooftreemodel.h \
    htmldelegate.h \
    httpd.h \
    webendpoint.h

DISTFILES += \
    README

FORMS += \
    mainwindow.ui
