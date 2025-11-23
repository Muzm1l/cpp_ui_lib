QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# Fix for macOS AGL framework issue - use modern OpenGL instead
macx {
    QMAKE_LFLAGS += -framework OpenGL
    LIBS += -framework OpenGL
}

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.

SOURCES += \
    graphcontainer.cpp \
    graphlayout.cpp \
    graphtype.cpp \
    main.cpp \
    mainwindow.cpp \
    tacticalsolutionview.cpp \
    timelinedrawingobjects.cpp \
    timelineview.cpp \
    timeselectionvisualizer.cpp \
    twoaxisdata.cpp \
    twoaxisgraph.cpp \
    waterfalldata.cpp \
    waterfallgraph.cpp \
    drawutils.cpp \
    zoompanel.cpp \
    bdwgraph.cpp \
    brwgraph.cpp \
    btwgraph.cpp \
    fdwgraph.cpp \
    ftwgraph.cpp \
    ltwgraph.cpp \
    rtwgraph.cpp \
    rtwsymbol.cpp \
    simulator.cpp \
    interactivegraphicsitem.cpp \
    btwinteractiveoverlay.cpp 

HEADERS += \
    graphcontainer.h \
    graphlayout.h \
    graphtype.h \
    mainwindow.h \
    tacticalsolutionview.h \
    timelinedrawingobjects.h \
    timelineview.h \
    timeselectionvisualizer.h \
    twoaxisdata.h \
    twoaxisgraph.h \
    waterfalldata.h \
    waterfallgraph.h \
    drawutils.h \
    zoompanel.h\
    bdwgraph.h \
    brwgraph.h \
    btwgraph.h \
    fdwgraph.h \
    ftwgraph.h \
    ltwgraph.h \
    rtwgraph.h \
    rtwsymbols.h \
    simulator.h \
    interactivegraphicsitem.h \
    btwinteractiveoverlay.h 

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
