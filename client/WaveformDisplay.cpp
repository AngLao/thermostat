#include "WaveformDisplay.h"


WaveformDisplay::WaveformDisplay(QWidget *parent)
    : QWidget{parent}
{
    //定义颜色
    userColorStrList<<"gold"<<"darkcyan"<<"steelblue"<<"deepskyblue"
                     <<"darkslateblue"<<"brown"<<"forestgreen"
                     <<"gold"<<"darkorange"<<"orangered"<<"violet"
                     <<"darkviolet"<<"darkgoldenrod"<<"deeppink";


    //提升widget类为QCustomPlot类
    pQCustomPlot = new QCustomPlot();

    //新增固定波形
    for (int var = 0; var < 2; ++var) {
        QPen pen;
        pen.setWidth(2);//设置线宽
        pen.setStyle(Qt::PenStyle::SolidLine);//设置为实线
        pen.setColor(QColor(userColorStrList.at(var)));//设置线条颜色

        pQCustomPlot->addGraph();
        pQCustomPlot->graph(var)->setPen(pen);
        pQCustomPlot->graph(var)->setVisible(true);
    }

    //显示图例
    pQCustomPlot->legend->setVisible(true);
    pQCustomPlot->legend->setBrush(QColor(255, 255, 255, 150));
    //设置名称
    pQCustomPlot->graph(0)->setName("测量温度: "+QString::number(0));
    pQCustomPlot->graph(1)->setName("设定温度: "+QString::number(0));
    //允许拖拽
    pQCustomPlot->setInteraction( QCP::iRangeDrag , true);
    //允许缩放
    pQCustomPlot->setInteraction( QCP::iRangeZoom , true);

    //底部水平布局装控制按钮
    QFrame* bottomLayoutFrame = new QFrame();
    QHBoxLayout* bottomLayout = new QHBoxLayout(bottomLayoutFrame);
    bottomLayout->setContentsMargins(2,2,2,2);
    bottomLayout->setSpacing(2);
    //配置按钮
    //开始显示按钮
    QPushButton* startWaveformButton = new QPushButton();
    startWaveformButton->setText("暂停显示");
    connect(startWaveformButton,&QPushButton::clicked,pQCustomPlot,[=](){
        if(startWaveformButton->text() == "显示曲线"){
            startWaveformButton->setText("暂停显示");
            startFlag = true;
        }else{
            startWaveformButton->setText("显示曲线");
            startFlag = false;
        }
    });
    //清空波形按钮
    bottomLayout->addWidget(startWaveformButton);
    QPushButton* clearWaveformButton = new QPushButton();
    clearWaveformButton->setText("清空波形");
    connect(clearWaveformButton,&QPushButton::clicked,pQCustomPlot,[=](){
        int count=pQCustomPlot->graphCount();//获取曲线条数
        for(int i=0;i<count;++i)
        {
            pQCustomPlot->graph(i)->data().data()->clear();
        }
        x = 0;
        pQCustomPlot->replot();
    });
    bottomLayout->addWidget(clearWaveformButton);
    //动态显示按钮
    QPushButton* dynamicDisplayButton = new QPushButton();
    dynamicDisplayButton->setText("停止跟随");
    connect(dynamicDisplayButton,&QPushButton::clicked,pQCustomPlot,[=](){
        if(dynamicDisplayButton->text() == "动态显示"){
            dynamicDisplayButton->setText("停止跟随");
            dynamicDisplayFlag = true;
        }else{
            dynamicDisplayButton->setText("动态显示");
            dynamicDisplayFlag = false;
        }
    });
    bottomLayout->addWidget(dynamicDisplayButton);
    //全显波形按钮
    QPushButton* seeAllLineButton = new QPushButton();
    seeAllLineButton->setText("波形全显");
    connect(seeAllLineButton,&QPushButton::clicked,pQCustomPlot,[=](){
        dynamicDisplayButton->setText("动态显示");
        dynamicDisplayFlag = false;
        pQCustomPlot->xAxis->rescale(true);//调整X轴的范围，使之能显示出所有的曲线的X值
        pQCustomPlot->yAxis->rescale(true);//调整Y轴的范围，使之能显示出所有的曲线的Y值
        pQCustomPlot->replot();  // 刷新
    });
    bottomLayout->addWidget(seeAllLineButton);

    //主布局设置
    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(2,0,2,0);
    mainLayout->setSpacing(2);
    mainLayout->addWidget(pQCustomPlot,5);
    mainLayout->addWidget(bottomLayoutFrame,1);

    waveformViewWidget = new QWidget();
    waveformViewWidget->setLayout(mainLayout);

    //    //设置波形显示背景
    //    QLinearGradient plotGradient;
    //    plotGradient.setStart(0, 0);
    //    plotGradient.setFinalStop(0, 350);
    //    plotGradient.setColorAt(0, QColor("#20B2AA"));
    //    plotGradient.setColorAt(1, QColor("#FFFACD"));
    //    pQCustomPlot->setBackground(plotGradient);
}

QWidget* WaveformDisplay::widget()
{
    return waveformViewWidget;
}



void WaveformDisplay::paintUserData(double x ,double y , int num)
{
    //获取checkbox控件句柄
    static QList<QCheckBox*>  CheckBoxList = this->widget()->findChildren<QCheckBox*>();

    if(startFlag){
        //添加数据
        pQCustomPlot->graph(num)->addData(x,y);
    }
    if(dynamicDisplayFlag){
        //刷新数据的时候才动态显示
        //动态x轴
        pQCustomPlot->xAxis->setRange(x-20, 300, Qt::AlignLeft);
        //设置y轴范围
        QCPRange a = pQCustomPlot->yAxis->range();
        if(y < a.lower){
            pQCustomPlot->yAxis->setRange(y,a.upper);
        }
        if(y > a.upper){
            pQCustomPlot->yAxis->setRange(a.lower,y);
        }
    }

    pQCustomPlot->replot(QCustomPlot::rpQueuedReplot); //刷新

}

void WaveformDisplay::ProcessingMessages(uint8_t *pData, uint8_t len)
{
    uint8_t id = *(pData+2)-48;
    uint32_t data = 0 ;

    for (int var = 0; var < 4; var++) {
        data += *(pData+4+var)<<(8*(3-var));
    }
    //缩放100倍
    double res = int(data)/100.0;

    //设置名称
    switch (id) {
    case 0x00:
        pQCustomPlot->graph(0)->setName("测量温度: "+QString::number(res));
        break;
    case 0x01:
        pQCustomPlot->graph(1)->setName("设定温度: "+QString::number(res));
        break;
    default:
        break;
    }
    paintUserData(x, res  , id);
    x++;
}

void WaveformDisplay::ProcessMqttMessages(QString topic, double value)
{
    //设置名称
    if(topic == "measure"){
        pQCustomPlot->graph(0)->setName("测量温度: "+QString::number(value));
        paintUserData(x, value  , 0);
    }else if(topic == "trage"){
        pQCustomPlot->graph(1)->setName("设定温度: "+QString::number(value));
        paintUserData(x, value  , 1);
    }
    x++;
}
