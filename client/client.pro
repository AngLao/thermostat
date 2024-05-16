QT       += core gui
QT       += serialport      #加入串口模块
QT       += printsupport    #加入画图模块
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

SOURCES += \
    MainWindows.cpp \
    WaveformDisplay.cpp \
    EasyPact.c \
    main.cpp

HEADERS += \
    MainWindows.h \
    WaveformDisplay.h \
    EasyPact.h

FORMS += \
    MainWindows.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

#波形图qcustomplot2库路径
LIBS += -L$$PWD/qcustomplot -lqcustomplot2
INCLUDEPATH += $$PWD/qcustomplot
DEPENDPATH += $$PWD/qcustomplot

#mqtt库路径
LIBS += -L$$PWD/QtMqtt -lQt6Mqtt
INCLUDEPATH += $$PWD/QtMqtt
DEPENDPATH += $$PWD/QtMqtt
