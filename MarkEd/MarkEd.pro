QT += core gui webenginewidgets printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

DEFINES += QT_DEPRECATED_WARNINGS

LIBS += -lmd4c

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    mdrenderer.cpp \
    highlighter.cpp

HEADERS += \
    mainwindow.h \
    mdrenderer.h \
    highlighter.h

FORMS += \
    mainwindow.ui

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
