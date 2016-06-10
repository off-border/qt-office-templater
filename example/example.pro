
QT += core gui script scripttools xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = templater_example

TEMPLATE = app

SOURCES += \
    main.cpp \
    example.cpp

HEADERS += \
    example.h


INCLUDEPATH += $$PWD/../../libs/quazip

LIBS        += -L$$PWD/../../../bin/ -lquazip

include( ../templater.pri )

