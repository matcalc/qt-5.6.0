QT += serialbus widgets

TARGET = modbusslave
TEMPLATE = app
CONFIG += c++11

SOURCES += main.cpp\
        mainwindow.cpp \
        settingsdialog.cpp

HEADERS  += mainwindow.h settingsdialog.h

FORMS    += mainwindow.ui settingsdialog.ui

RESOURCES += slave.qrc
