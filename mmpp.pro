
# Comment this to enable QT
CONFIG -= qt

CONFIG(qt) {
    QT       += core gui
    greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
    DEFINES += USE_QT
}

TEMPLATE = app
CONFIG += link_pkgconfig
PKGCONFIG += libmicrohttpd libcrypto++

SOURCES += main.cpp \
    library.cpp \
    proof.cpp \
    unification.cpp \
    wff.cpp \
    toolbox.cpp \
    test.cpp \
    utils.cpp \
    httpd.cpp \
    web.cpp \
    platform.cpp \
    workset.cpp \
    jsonize.cpp \
    z3prover.cpp \
    reader.cpp \
    main_unification.cpp \
    temp.cpp

CONFIG(qt) {
SOURCES +=  mainwindow.cpp \
    prooftreemodel.cpp \
    htmldelegate.cpp \
    main_qt.cpp
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

QMAKE_CFLAGS += -std=c11 -march=native -g
QMAKE_CXXFLAGS += -std=c++17 -march=native -g -ftemplate-backtrace-limit=0
QMAKE_LIBS += -ldl -export-dynamic -rdynamic -lboost_system -lboost_filesystem -lboost_serialization -lpthread -lz3

HEADERS += \
    wff.h \
    library.h \
    proof.h \
    unification.h \
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
    z3prover.h \
    lr.h \
    reader.h \
    parser.h \
    serialize_tuple.h \
    unif.h

CONFIG(qt) {
HEADERS +=  mainwindow.h \
    prooftreemodel.h \
    htmldelegate.h
}

DISTFILES += \
    README

FORMS += \
    mainwindow.ui

create_links.commands = for i in mmpp_verify_one mmpp_verify_all mmpp_test_setmm mmpp_unificator webmmpp qmmpp mmpp_test_z3 ; do ln -s mmpp \$\$i 2>/dev/null || true ; done
QMAKE_EXTRA_TARGETS += create_links
POST_TARGETDEPS += create_links
