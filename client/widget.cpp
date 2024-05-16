#include "widget.h"
#include <QApplication>
#include <QTextEdit>
#include <QToolButton>

#include "SerialPortBase.h"
#include "WaveformDisplay.h"
#include "ParameterConfiguration.h"

#include "QtMqtt/qmqttclient.h"


//主函数入口
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Widget w;
    w.show();

    return a.exec();
}


Widget::Widget(QWidget *parent) : QWidget(parent)
{
    this->resize(940, 450);
    QMetaObject::connectSlotsByName(this);
    //主窗口初始化

    //添加自定义界面
    SerialPortBase* SerialPortBase = new class SerialPortBase(this);
    WaveformDisplay* WaveformDisplay = new class WaveformDisplay();
    ParameterConfiguration* ParameterConfiguration = new class ParameterConfiguration();


    QHBoxLayout* TopLayout = new QHBoxLayout();
    TopLayout->addWidget(SerialPortBase->widget(),4);
    TopLayout->addWidget(ParameterConfiguration->widget(),1);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    //设置布局边距Policy
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);
    mainLayout->addLayout(TopLayout);
    mainLayout->addWidget(WaveformDisplay->widget());

    //设置布局排版
    mainLayout->setStretch(1,5);
    this->setLayout(mainLayout);

    //配置数据发送链接
    connect(ParameterConfiguration, &ParameterConfiguration::sendPackData,SerialPortBase,&SerialPortBase::SendData);
    //显示波形
    connect(SerialPortBase, &SerialPortBase::drawWaveform, WaveformDisplay, &WaveformDisplay::ProcessingMessages);

    QMqttClient *m_client;
    m_client = new QMqttClient(this);
    m_client->setHostname("107.173.179.48");
    m_client->setPort(1883);
    m_client->setClientId("pc");
    m_client->setUsername("上位机");
    m_client->setPassword("anglao666");
    m_client->connectToHost();

    // connect(m_client, &QMqttClient::stateChanged, this, &MainWindow::updateLogStateChange);
    connect(m_client, &QMqttClient::connected, this, [=](){
        m_client->subscribe(QString("trage"));
        m_client->subscribe(QString("measure"));
        qDebug()<<"mqtt 服务器连接成功";
    });

    connect(m_client, &QMqttClient::messageReceived, this, [=](const QByteArray &message, const QMqttTopicName &topic) {
        qDebug()<< QLatin1String(" Received Topic: ")
                                + topic.name()
                                + QLatin1String(" Message: ")
                                + message
                                + QLatin1Char('\n');
        WaveformDisplay->ProcessMqttMessages(topic.name(),message.toFloat());
    });


}



