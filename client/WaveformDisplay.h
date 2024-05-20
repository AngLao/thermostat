
#ifndef WAVEFORMDISPLAY_H
#define WAVEFORMDISPLAY_H


#include <qcustomplot.h>
#include <QObject>


class WaveformDisplay : public QWidget
{
    Q_OBJECT
public:
    explicit WaveformDisplay(QWidget *parent = nullptr);
    QStringList userColorStrList;
    QCustomPlot* pQCustomPlot;
    //按钮标志位
    bool startFlag = true;
    bool dynamicDisplayFlag =true;

    QWidget *widget();
    void ProcessingMessages(uint8_t*pData ,uint8_t len);
    void ProcessMqttMessages(QString topic, double value);
private:
    QWidget* waveformViewWidget;
    void paintUserData(double x ,double y , int num);
    void lineInit();

    long long int x1=0, x2=0;
signals:

};

#endif // WAVEFORMDISPLAY_H
