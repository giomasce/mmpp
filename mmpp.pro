
CONFIG -= qt

CONFIG(qt) {
    QT       += core gui
    greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
    DEFINES += USE_QT
}

TEMPLATE = app
CONFIG += link_pkgconfig
PKGCONFIG += libmicrohttpd

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
    web.cpp \
    platform.cpp \
    workset.cpp \
    jsonize.cpp \
    z3prover.cpp

CONFIG(qt) {
SOURCES +=  mainwindow.cpp \
    prooftreemodel.cpp \
    htmldelegate.cpp
}

# Experiments with clang
#QMAKE_CC = clang
#QMAKE_CXX = clang++
#QMAKE_LINK = clang++
#QMAKE_LIBS += -ldl -rdynamic -lboost_system -lboost_filesystem -lpthread -fsanitize=undefined

# Experiments with undefined behavior checking
#QMAKE_CFLAGS += -fsanitize=undefined -fno-sanitize-recover=all
#QMAKE_CXXFLAGS += -std=c++17 -march=native -fsanitize=undefined -fno-sanitize-recover=all
#QMAKE_LIBS += -ldl -export-dynamic -rdynamic -lboost_system -lboost_filesystem -lpthread -fsanitize=undefined -fno-sanitize-recover=all

QMAKE_CFLAGS += -std=c11
QMAKE_CXXFLAGS += -std=c++17 -march=native
QMAKE_LIBS += -ldl -export-dynamic -rdynamic -lboost_system -lboost_filesystem -lpthread -lz3

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
    web.h \
    json.h \
    platform.h \
    workset.h \
    jsonize.h \
    z3prover.h

CONFIG(qt) {
HEADERS +=  mainwindow.h \
    prooftreemodel.h \
    htmldelegate.h
}

DISTFILES += \
    README

FORMS += \
    mainwindow.ui

create_links.commands = for i in mmpp_test_one mmpp_test_all unificator webmmpp qmmpp ; do ln -s mmpp \$\$i 2>/dev/null || true ; done
QMAKE_EXTRA_TARGETS += create_links
POST_TARGETDEPS += create_links
