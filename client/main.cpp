#include <QApplication>
#include <QFile>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "MainWindows.h"
#include "WaveformDisplay.h"

#include "QtMqtt/qmqttclient.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QWidget *windows = new QWidget;

    windows->resize(940, 450);
    //主窗口初始化

    //添加自定义界面
    MainWindows* MainWindows = new class MainWindows(windows);
    WaveformDisplay* WaveformDisplay = new class WaveformDisplay();

    QVBoxLayout* mainLayout = new QVBoxLayout(windows);
    //设置布局边距Policy
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(MainWindows->widget());
    mainLayout->addWidget(WaveformDisplay->widget());

    //设置布局排版
    mainLayout->setStretch(1,5);
    windows->setLayout(mainLayout);

    //显示波形
    QObject::connect(MainWindows, &MainWindows::DrawSerialData, WaveformDisplay, &WaveformDisplay::ProcessingMessages);
    QObject::connect(MainWindows, &MainWindows::DrawMqttData, WaveformDisplay, &WaveformDisplay::ProcessMqttMessages);

    windows->show();
    return a.exec();
}
