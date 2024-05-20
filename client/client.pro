QT       += core gui
QT       += serialport      #加入串口模块
QT       += printsupport    #加入画图模块
QT       += network websockets

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

#qcustomplot2库
win32: LIBS += -L$$PWD/qcustomplot/ -lqcustomplot2
unix:!macx: LIBS += -L$$PWD/qcustomplot/ -lqcustomplot_arm64-v8a

INCLUDEPATH += $$PWD/qcustomplot
DEPENDPATH += $$PWD/qcustomplot

#emqx mqtt库
win32: LIBS += -L$$PWD/emqx -lQt6Qmqtt
unix:!macx: LIBS += -L$$PWD/emqx/ -lQt6Qmqtt_arm64-v8a
INCLUDEPATH += $$PWD/emqx
DEPENDPATH += $$PWD/emqx

#ssl库
win32: LIBS += -L$$PWD/ssl/windows -llibcrypto-1_1-x64
win32: LIBS += -L$$PWD/ssl/windows -llibssl-1_1-x64

#android apk打包库
android: include(D:/work/Qt/android/android_openssl/openssl.pri)

contains(ANDROID_TARGET_ARCH,arm64-v8a) {
    ANDROID_EXTRA_LIBS = \
        $$PWD/ssl/arm64-v8a/libcrypto_1_1.so \
        $$PWD/ssl/arm64-v8a/libssl_1_1.so \
        $$PWD/emqx/libQt6Qmqtt_arm64-v8a.so \
        $$PWD/qcustomplot/libqcustomplot_arm64-v8a.so

    ANDROID_PACKAGE_SOURCE_DIR = \
        $$PWD/android
}
