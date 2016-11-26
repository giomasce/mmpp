TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    parser.cpp \
    library.cpp \
    proof.cpp \
    unification.cpp

QMAKE_CXXFLAGS += -std=c++17 -march=native

HEADERS += \
    wff.h \
    parser.h \
    library.h \
    statics.h \
    proof.h \
    unification.h
