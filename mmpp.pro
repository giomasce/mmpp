
# Enable or disable various components
USE_QT = false
USE_MICROHTTPD = true
USE_BEAST = false
USE_Z3 = true

# Some features of Boost.Test are relatively recent, so in order to support
# older distributions we make it easy to disable tests altogether.
DISABLE_TESTS = false

TEMPLATE = app
CONFIG += link_pkgconfig
#CONFIG += object_parallel_to_source

#CONFIG += precompile_header
#PRECOMPILED_HEADER += pch.h

DEFINES += "PROJ_DIR=\"\\\"$$PWD\\\"\""

CONFIG += c++1z
CONFIG -= app_bundle

INCLUDEPATH += $$PWD/libs/giolib

linux {
    # Compile with gcc
    #QMAKE_CC = gcc
    #QMAKE_CXX = g++
    #QMAKE_LINK = g++
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
    #QMAKE_CC = clang
    #QMAKE_CXX = clang++
    #QMAKE_LINK = clang++
    QMAKE_CFLAGS += -std=c11 -g -I/usr/local/include
    QMAKE_CXXFLAGS += -g -ftemplate-backtrace-limit=0 -I/usr/local/include
    QMAKE_LIBS += -ldl -rdynamic -L/usr/local/lib
}

equals(DISABLE_TESTS, "true") {
    DEFINES += DISABLE_TESTS
}

!win32 {
    QMAKE_LIBS += -lboost_system -lboost_filesystem -lboost_serialization
    !equals(DISABLE_TESTS, "true") {
        QMAKE_LIBS += -lboost_unit_test_framework
    }
}

linux {
    QMAKE_LIBS += -pthread -lbfd -lboost_context
}

macx {
    QMAKE_LIBS += -lboost_coroutine
}

win32 {
    # See comment in test/test.cpp
    #DEFINES += TEST_BUILD
    DEFINES += BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE
    DEFINES += NOMINMAX
    #DEFINES += BOOST_LIB_DIAGNOSTIC
    INCLUDEPATH += c:\local\boost_1_69_0
    INCLUDEPATH += c:\libs
    QMAKE_CXXFLAGS += /wd4250 /wd4200 /Zi /std:c++17
    QMAKE_LFLAGS += /STACK:8388608 /DEBUG:FULL
    LIBS += -Lc:\libs -Lc:\local\boost_1_69_0\lib64-msvc-14.1
    CONFIG += console
}

# Disable these to have faster code; enable them to spot bugs
#DEFINES += LR_PARSER_SELF_TEST
#DEFINES += UNIFICATOR_SELF_TEST
#DEFINES += TOOLBOX_SELF_TEST

# Trick from https://stackoverflow.com/a/21335126/807307
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
#QMAKE_CXXFLAGS_RELEASE *= -O3
QMAKE_CXXFLAGS_RELEASE *= -Og

# Tricks that (in theory) boost execution speed (for GCC and LLVM)
#QMAKE_CFLAGS += -march=native -mtune=native -flto
#QMAKE_CXXFLAGS += -march=native -mtune=native -flto
#QMAKE_LFLAGS += -march=native -mtune=native -flto

# Enable loop vectorization
#QMAKE_CFLAGS += -ftree-vectorize -ftree-vectorizer-verbose=2
#QMAKE_CXXFLAGS += -ftree-vectorize -ftree-vectorizer-verbose=2
#QMAKE_LFLAGS += -ftree-vectorize -ftree-vectorizer-verbose=2

# Undefined behavior checking (for GCC and LLVM)
#QMAKE_CFLAGS += -fsanitize=undefined -fno-sanitize-recover=all
#QMAKE_CXXFLAGS += -fsanitize=undefined -fno-sanitize-recover=all
#QMAKE_LIBS += -fsanitize=undefined -fno-sanitize-recover=all

# Address checking (for GCC and LLVM)
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
    apps/generalizable_theorems.cpp \
    test/test_parsing.cpp \
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
    provers/tstp/tstp.cpp \
    provers/wffsat.cpp \
    apps/resolver.cpp \
    provers/subst.cpp \
    apps/verify.cpp \
    test/test_wff.cpp \
    mm/mmutils.cpp \
    provers/tstp/tstp_parser.cpp \
    provers/z3prover3.cpp \
    provers/z3resolver.cpp \
    provers/gapt.cpp \
    provers/fof.cpp \
    mm/setmm_loader.cpp \
    provers/fof_to_mm.cpp \
    provers/setmm.cpp \
    provers/ndproof.cpp \
    provers/ndproof_to_mm.cpp

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
    provers/wffblock.h \
    provers/wffsat.h \
    provers/subst.h \
    test/test.h \
    mm/mmutils.h \
    provers/tstp/tstp_parser.h \
    provers/z3prover3.h \
    provers/z3resolver.h \
    provers/gapt.h \
    provers/fof.h \
    mm/setmm_loader.h \
    provers/setmm.h \
    provers/fof_to_mm.h \
    provers/ndproof.h \
    provers/ndproof_to_mm.h

DISTFILES += \
    README.md \
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
    linux {
        PKGCONFIG += libmicrohttpd
    }
    macx {
        QMAKE_LIBS += -lmicrohttpd
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
        provers/z3prover.cpp \
        provers/z3prover2.cpp
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
