#ifndef MAINWINDOWS_H
#define MAINWINDOWS_H

#include <QWidget>
#include <QMouseEvent>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QTime>
#include <QTimer>
#include <QComboBox>
#include <QMouseEvent>
#include <QTextEdit>

#include "EasyPact.h"
#include "QtMqtt/qmqttclient.h"

namespace Ui {
class MainWindows;
}

class MainWindows : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindows(QWidget *parent = nullptr);
    ~MainWindows();

    QWidget* widget();
    void SerialSendData(const char *, const int);
private:
    Ui::MainWindows *ui;
    QSerialPort *serialPort;
    QMqttClient *mqttClient;
    bool eventFilter(QObject *, QEvent *);
    void SerialInit();
    void MqttInit();
    void TemperatureConfigInit();
signals:
    void ClickBox();
    void sendPackData(const char *data , const int DataLen);
    void DataReady(QByteArray& ReceiveData);
    void RecivePact(uint8_t* pData  ,uint8_t len);
    void DrawSerialData(uint8_t*pData ,uint8_t len);
    void DrawMqttData(QString topic, double value);
};

#endif // MAINWINDOWS_H
