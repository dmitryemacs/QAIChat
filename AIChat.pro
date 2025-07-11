QT       += core gui network widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    SyntaxHighlighter.cpp \
    settingsdialog.cpp

HEADERS += \
    mainwindow.h \
    SyntaxHighlighter.h \
    settingsdialog.h

FORMS += \
    mainwindow.ui
