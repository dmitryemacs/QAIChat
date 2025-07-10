QT       += core gui network widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17 # Рекомендуется использовать современный стандарт C++

# The following defines go into the PCH file (if any).
# For example, include "config.h" in your main.cpp
# to get these definitions.
DEFINES += QT_DEPRECATED_WARNINGS

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only for a specific module:
# DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui # Если вы используете UI-файл, он будет здесь. Если нет, удалите эту строку.

# Default rules for deployment.
# qnx: target.path = /tmp/$${TARGET}/bin
# else: target.path = /opt/$${TARGET}/bin
# INSTALLS += target
