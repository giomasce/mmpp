
# Enable or disable various components
USE_QT = true
USE_MICROHTTPD = true
USE_BEAST = true
USE_Z3 = true

TEMPLATE = app
CONFIG += link_pkgconfig
#CONFIG += object_parallel_to_source

#CONFIG += precompile_header
#PRECOMPILED_HEADER += pch.h

CONFIG += c++1z

linux {
    # Compile with gcc
    QMAKE_CC = gcc
    QMAKE_CXX = g++
    QMAKE_LINK = g++
    QMAKE_CFLAGS += -std=c11 -g
    QMAKE_CXXFLAGS += -g -ftemplate-backtrace-limit=0
    QMAKE_LIBS += -ldl -export-dynamic -rdynamic

    # Compile with clang
    #QMAKE_CC = clang-7
    #QMAKE_CXX = clang++-7
    #QMAKE_LINK = clang++-7
    #QMAKE_CFLAGS += -std=c11 -g
    #QMAKE_CXXFLAGS += -g -ftemplate-backtrace-limit=0
    #QMAKE_LIBS += -ldl -rdynamic
}

macx {
    QMAKE_CC = clang
    QMAKE_CXX = clang++
    QMAKE_LINK = clang++
    QMAKE_CFLAGS += -std=c11 -g -I/usr/local/include
    QMAKE_CXXFLAGS += -g -ftemplate-backtrace-limit=0 -I/usr/local/include
    QMAKE_LIBS += -ldl -rdynamic -L/usr/local/lib
}

!win32 {
    QMAKE_LIBS += -lboost_system -lboost_filesystem -lboost_serialization -lboost_coroutine -lpthread
}

win32 {
    QMAKE_CXXFLAGS += /std:c++17 /I c:\Boost\include\boost-1_66 /I c:\libs /FC
    QMAKE_LFLAGS += /STACK:8388608
    QMAKE_LIBS += /LIBPATH:c:\Boost\lib /LIBPATH:c:\libs
    CONFIG += console
}

# Trick from https://stackoverflow.com/a/21335126/807307
#QMAKE_CXXFLAGS_RELEASE -= -O1
#QMAKE_CXXFLAGS_RELEASE -= -O2
#QMAKE_CXXFLAGS_RELEASE *= -O3

# Compile to native instruction set
#QMAKE_CFLAGS += -march=native
#QMAKE_CXXFLAGS += -march=native

# Disable these to have faster code; enable them to spot bugs
#QMAKE_CXXFLAGS += -DLR_PARSER_SELF_TEST
#QMAKE_CXXFLAGS += -DUNIFICATOR_SELF_TEST
#QMAKE_CXXFLAGS += -DTOOLBOX_SELF_TEST

# Experiments with undefined behavior checking
#QMAKE_CFLAGS += -fsanitize=undefined -fno-sanitize-recover=all
#QMAKE_CXXFLAGS += -fsanitize=undefined -fno-sanitize-recover=all
#QMAKE_LIBS += -fsanitize=undefined -fno-sanitize-recover=all

# Experiments with address checking
#QMAKE_CFLAGS += -fsanitize=address -fno-sanitize-recover=all -fsanitize-address-use-after-scope
#QMAKE_CXXFLAGS += -fsanitize=address -fno-sanitize-recover=all -fsanitize-address-use-after-scope
#QMAKE_LIBS += -fsanitize=address -fno-sanitize-recover=all -fsanitize-address-use-after-scope

SOURCES += \
    main.cpp \
    mm/library.cpp \
    mm/proof.cpp \
    old/unification.cpp \
    provers/wff.cpp \
    mm/toolbox.cpp \
    test/test.cpp \
    utils/utils.cpp \
    web/web.cpp \
    platform.cpp \
    web/workset.cpp \
    web/jsonize.cpp \
    mm/reader.cpp \
    apps/unificator.cpp \
    temp.cpp \
    test/test_env.cpp \
    apps/generalizable_theorems.cpp \
    test/test_parsing.cpp \
    test/test_verification.cpp \
    test/test_minor.cpp \
    web/step.cpp \
    utils/threadmanager.cpp \
    apps/learning.cpp \
    provers/uct.cpp \
    mm/tokenizer.cpp \
    mm/engine.cpp \
    mm/funds.cpp \
    mm/mmtemplates.cpp \
    mm/ptengine.cpp \
    mm/sentengine.cpp \
    web/strategy.cpp \
    mm/tempgen.cpp \
    libs/minisat/Solver.cc \
    provers/sat.cpp \
    provers/wffblock.cpp \
    apps/tstp.cpp

HEADERS += \
    pch.h \
    provers/wff.h \
    mm/library.h \
    mm/proof.h \
    old/unification.h \
    mm/toolbox.h \
    utils/stringcache.h \
    utils/utils.h \
    web/httpd.h \
    web/web.h \
    libs/json.h \
    platform.h \
    web/workset.h \
    web/jsonize.h \
    mm/reader.h \
    libs/serialize_tuple.h \
    test/test_env.h \
    test/test_parsing.h \
    test/test_verification.h \
    test/test_minor.h \
    utils/vectormap.h \
    parsing/parser.h \
    parsing/earley.h \
    parsing/lr.h \
    parsing/unif.h \
    web/step.h \
    utils/threadmanager.h \
    utils/backref_registry.h \
    parsing/algos.h \
    provers/uct.h \
    mm/tokenizer.h \
    mm/engine.h \
    mm/funds.h \
    mm/mmtypes.h \
    mm/mmtemplates.h \
    mm/ptengine.h \
    mm/sentengine.h \
    web/strategy.h \
    mm/tempgen.h \
    libs/minisat/Alg.h \
    libs/minisat/Alloc.h \
    libs/minisat/Heap.h \
    libs/minisat/IntTypes.h \
    libs/minisat/Map.h \
    libs/minisat/Solver.h \
    libs/minisat/SolverTypes.h \
    libs/minisat/Sort.h \
    libs/minisat/Vec.h \
    libs/minisat/XAlloc.h \
    provers/sat.h \
    provers/wffblock.h

DISTFILES += \
    README \
    tests.txt \
    resources/tests.txt

!equals(USE_QT, "true") {
    CONFIG -= qt
}

equals(USE_QT, "true") {
    DEFINES += USE_QT
    QT += core gui
    greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
    SOURCES += \
        qt/mainwindow.cpp \
        qt/prooftreemodel.cpp \
        qt/htmldelegate.cpp \
        qt/main_qt.cpp
    HEADERS += \
        qt/mainwindow.h \
        qt/prooftreemodel.h \
        qt/htmldelegate.h
    FORMS += \
        qt/mainwindow.ui
}

equals(USE_MICROHTTPD, "true") {
    DEFINES += USE_MICROHTTPD
    SOURCES += \
        web/httpd_microhttpd.cpp
    HEADERS += \
        web/httpd_microhttpd.h
    !win32 {
        PKGCONFIG += libmicrohttpd
    }
    win32 {
        QMAKE_LIBS += -llibmicrohttpd
    }
}

equals(USE_BEAST, "true") {
    DEFINES += USE_BEAST
    SOURCES += \
        web/httpd_beast.cpp
    HEADERS += \
        web/httpd_beast.h
}

equals(USE_Z3, "true") {
    DEFINES += USE_Z3
    SOURCES += \
        provers/z3prover.cpp
    !win32 {
        QMAKE_LIBS += -lz3
    }
    win32 {
        QMAKE_LIBS += -llibz3
    }
}

#create_links.commands = for i in mmpp_dissector mmpp_gen_random_theorems mmpp_verify_one mmpp_simple_verify_one mmpp_verify_all mmpp_test_setmm mmpp_unificator webmmpp webmmpp_open qmmpp mmpp_test_z3 mmpp_generalizable_theorems ; do ln -s mmpp \$\$i 2>/dev/null || true ; done
#QMAKE_EXTRA_TARGETS += create_links
#POST_TARGETDEPS += create_links
