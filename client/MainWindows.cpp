#include "MainWindows.h"
#include "ui_MainWindows.h"

#include "EasyPact.h"
#include "emqx/qmqtt_client.h"
#include "emqx/qmqtt_message.h"

MainWindows::MainWindows(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWindows)
{
    ui->setupUi(this);
    SerialInit();
    MqttInit();
    TemperatureConfigInit();
}

MainWindows::~MainWindows()
{
    delete ui;
}

QWidget *MainWindows::widget()
{
    return ui->BaudRateComboBox->parentWidget();
}

bool MainWindows::eventFilter(QObject *obj, QEvent *event)
{
    if(event->type() == QEvent::MouseButtonPress)
        if(obj == ui->SerialPortChooseComboBox)
            emit ClickBox();

    return QWidget::eventFilter(obj, event);
}

void MainWindows::SerialInit()
{
    serialPort = new QSerialPort();

    //使选择下拉框具备点击响应,点击时进行一次搜寻串口操作
    ui->SerialPortChooseComboBox->installEventFilter(this);
    static auto FineSerialPort = [&](){
        QStringList SerialPortNameList;/*保存搜索到的串口，存入列表中*/
        ui->RxDataTextEdit->clear();
        ui->RxDataTextEdit->append("存在的串口：");
        foreach (const QSerialPortInfo &SerialPortInfo, QSerialPortInfo::availablePorts()) /*遍历可使用的串口*/
        {
            SerialPortNameList.append(SerialPortInfo.portName());/*把搜索到的串口存入列表中*/
            ui->RxDataTextEdit->append(SerialPortInfo.portName() + " " + SerialPortInfo.description());
        }
        ui->SerialPortChooseComboBox->clear();
        ui->SerialPortChooseComboBox->addItems(SerialPortNameList);/*将搜索到的串口显示到UI界面上*/
    };

    static auto DataHandle = [&](){
        static QByteArray SerialPortDataBuf ;
        static unsigned long recCount = 0;
        SerialPortDataBuf = serialPort->readAll();
        //刷新接收计数
        recCount+=SerialPortDataBuf.size();
        if(ui->RxDataForHexCheckBox->checkState() == 0)
        {
            ui->RxDataTextEdit->append(SerialPortDataBuf);
        }else{
            ui->RxDataTextEdit->append(SerialPortDataBuf.toHex(' ').toUpper());
        }

        //保持编辑器光标在最后一行
        ui->RxDataTextEdit->moveCursor(QTextCursor::End);
        //防止积累太多数据内存占用过高
        if(ui->RxDataTextEdit->toPlainText().size()>(1024*10)){
            ui->RxDataTextEdit->clear();
        }

        //解析是否存在自定义协议数据
        frame_t* pFrame = nullptr;
        for (int var = 0; var < SerialPortDataBuf.size(); ++var) {
            uint8_t res = easy_parse_data(&pFrame,(uint8_t)SerialPortDataBuf.at(var) );
            //解析到数据帧
            if( res == 0 && pFrame->address == 'W')
                emit DrawSerialData((uint8_t*)pFrame , easy_return_buflen(pFrame));
        }
    };

    //打开串口
    static auto OpenSerialPort = [&](){
        if(ui->OpenSerialPortPushButton->text() == "打开串口")
        {
            /*设置选中的COM口*/
            serialPort->setPortName(ui->SerialPortChooseComboBox->currentText());

            /*设置串口的波特率*/
            bool res = serialPort->setBaudRate(ui->BaudRateComboBox->currentText().toInt());
            if(res == false){
                ui->RxDataTextEdit->append("波特率设置失败");
                return;
            }
            /*设置数据位数*/
            serialPort->setDataBits( QSerialPort::DataBits(ui->PortDataBitsComboBox->currentText().toInt()) );

            /*设置奇偶校验,NoParit无校验*/
            int index = ui->PortParityComboBox->currentIndex();
            QSerialPort::Parity PortParityBits = (index == 0) ? (QSerialPort::NoParity) :QSerialPort::Parity(index+1);
            serialPort->setParity(PortParityBits);

            /*设置停止位，OneStop一个停止位*/
            serialPort->setStopBits( QSerialPort::StopBits((ui->PortStopBitsComboBox->currentIndex()+1)) );

            /*设置流控制，NoFlowControl无流控制*/
            serialPort->setFlowControl( QSerialPort::NoFlowControl );

            /*ReadWrite设置的是可读可写的属性*/
            if(serialPort->open(QIODevice::ReadWrite) == true){
                ui->RxDataTextEdit->append(ui->SerialPortChooseComboBox->currentText() + "已连接");

                ui->BaudRateComboBox->setEnabled(false);
                ui->PortStopBitsComboBox->setEnabled(false);
                ui->PortDataBitsComboBox->setEnabled(false);
                ui->PortParityComboBox->setEnabled(false);/*连接成功后设置ComboBox不可选择*/
                ui->SerialPortChooseComboBox->setEnabled(false);/*列表框无效*/

                /*打开串口成功，连接信号和槽*/
                ui->OpenSerialPortPushButton->setText("关闭串口");
            }else{
                ui->RxDataTextEdit->append(ui->SerialPortChooseComboBox->currentText() + "连接失败");
            }
        }else{
            /*关闭串口*/
            serialPort->close();
            ui->RxDataTextEdit->append(ui->SerialPortChooseComboBox->currentText() + "已关闭");
            ui->OpenSerialPortPushButton->setText("打开串口");
            ui->SerialPortChooseComboBox->setEnabled(true);
            ui->BaudRateComboBox->setEnabled(true);
            ui->PortStopBitsComboBox->setEnabled(true);
            ui->PortDataBitsComboBox->setEnabled(true);
            ui->PortParityComboBox->setEnabled(true);
        }
    };

    connect(this, &MainWindows::ClickBox, this,FineSerialPort);
    connect(serialPort, &QSerialPort::readyRead, this, DataHandle);
    //连接打开按钮按钮信号和槽
    connect( ui->OpenSerialPortPushButton, &QPushButton::clicked, this, OpenSerialPort);

    //清除接收框消息按钮信号和槽
    connect( ui->RxDataTextClearPushButton,&QPushButton::clicked,this,[&](){
        ui->RxDataTextEdit->clear();
    });

    //打开软件先搜索一次存在的串口
    FineSerialPort();
}

void MainWindows::MqttInit()
{
    QMQTT::Client *client = new QMQTT::Client;

    connect( ui->connect_mqtt,&QPushButton::clicked,this,[=](){
        if(ui->connect_mqtt->text() == "请求连接"){
            client->setVersion(QMQTT::V3_1_1);
            client->setHostName(ui->mqtt_ip->text());
            client->setPort(ui->mqtt_port->text().toInt());
            client->setClientId(ui->mqtt_id->text());
            client->setUsername(ui->mqtt_user->text());
            client->setPassword(ui->mqtt_password->text().toUtf8());
            client->setKeepAlive(60);
            client->connectToHost();
            ui->RxDataTextEdit->clear();
            ui->connect_mqtt->setText("断开连接");
        }else{
            client->disconnectFromHost();
            ui->connect_mqtt->setText("请求连接");
        }
    });

    connect(client, &QMQTT::Client::connected, this, [=](){
        client->subscribe(QString("trage"), 0);
        client->subscribe(QString("measure"), 0);
        ui->RxDataTextEdit->append("mqtt服务器连接成功");
    });

    connect(client, &QMQTT::Client::disconnected, this, [=](){
        ui->RxDataTextEdit->append("断开mqtt服务器连接");
        qDebug()<<"断开mqtt服务器连接";
    });

    connect(client, &QMQTT::Client::subscribed, this, [=](const QString& topic, const quint8 qos = 0){
        ui->RxDataTextEdit->append("topic:" + topic + "订阅成功");
    });

    connect(client, &QMQTT::Client::error, this, [=](const QMQTT::ClientError error){
        ui->RxDataTextEdit->append("error");
        qDebug()<<error;
    });

    connect(client, &QMQTT::Client::received, this, [=](const QMQTT::Message& message) {
        ui->RxDataTextEdit->append("Received Topic:" + message.topic() + ", Message:" + message.payload());
        emit DrawMqttData(message.topic(), message.payload().toFloat());
    });

    connect(this, &MainWindows::MqttSetTemp, this, [=](const float value) {
        QMQTT::Message message;
        message.setTopic("set_temp");
        message.setPayload(QByteArray::number(value));
        message.setRetain(1);
        client->publish(message);
    });


    connect(client, &QMQTT::Client::published, this, [=](const QMQTT::Message& message) {
        if(message.topic() == "set_temp")
            ui->RxDataTextEdit->append("设定目标温度：" + message.payload());
    });
}

void MainWindows::TemperatureConfigInit()
{
    static uint8_t TargetHeader;
    //目标帧头只能是一个字节
    ui->le_header->setValidator(new QIntValidator(0, 255));
    TargetHeader = REC_HEADER;
    connect(ui->le_header,&QLineEdit::editingFinished, this, [=]{
        TargetHeader = ui->le_header->text().toInt();
    });
    //带符号32位数值,规范QLineEdit输入格式
    ui->le_name->setValidator(new QRegularExpressionValidator(QRegularExpression("[a-zA-Z0-9]+")));
    ui->le_min->setValidator(new QIntValidator(-2147483647, 2147483647));
    ui->le_max->setValidator(new QIntValidator(-2147483647, 2147483647));
    ui->hs_set->setMinimum(ui->le_min->text().toInt()*100);
    ui->hs_set->setMaximum(ui->le_max->text().toInt()*100);
    ui->hs_set->setValue(0);

    static auto SetTrageTemperature = [&](){
    };

    //设定值改变时响应 打包数据串口发送命令
    connect(ui->sb_set, &QLineEdit::editingFinished,this,[=](){
        ui->hs_set->setValue(ui->sb_set->text().toDouble()*100);
        uint32_t data = ui->sb_set->text().toDouble()*100;

        //发送数据
        auto text = ui->le_name->text().toUtf8();
        static frame_t senbuf;
        easy_set_header(&senbuf, TargetHeader);
        easy_set_address(&senbuf, int(text.at(0)));
        easy_set_id(&senbuf, int(text.at(1)));

        easy_wipe_data(&senbuf);
        easy_add_data(&senbuf, data, 4);
        easy_add_check(&senbuf);
        //发送帧数据
        SerialSendData((char*)&senbuf ,easy_return_buflen(&senbuf));
        emit MqttSetTemp(ui->sb_set->text().toDouble());
    });

    //滑杆操作响应
    connect(ui->hs_set, &QSlider::sliderReleased,this,[=](){
        ui->sb_set->setText(QString::number(ui->hs_set->value()/100.0));

        uint32_t data = ui->sb_set->text().toDouble()*100;

        //发送数据
        auto text = ui->le_name->text().toUtf8();
        static frame_t senbuf;
        easy_set_header(&senbuf, TargetHeader);
        easy_set_address(&senbuf, int(text.at(0)));
        easy_set_id(&senbuf, int(text.at(1)));

        easy_wipe_data(&senbuf);
        easy_add_data(&senbuf, data, 4);
        easy_add_check(&senbuf);
        //发送帧数据
        SerialSendData((char*)&senbuf ,easy_return_buflen(&senbuf));
        emit MqttSetTemp(ui->sb_set->text().toDouble());
    });

    //地址ID改变时响应
    connect(ui->le_name, &QLineEdit::editingFinished,this,[=](){
        auto text = ui->le_name->text() ;
        if(text.length()!=2)
            ui->le_name->setText("T0");
    });
    //最大最小值编辑结束时响应
    connect(ui->le_max, &QLineEdit::editingFinished,this,[=](){
        int changvalue = ui->le_max->text().toInt()*100;
        int thisvalue = ui->sb_set->text().toDouble()*100;
        ui->hs_set->setMaximum(changvalue);
        ui->hs_set->setValue(thisvalue);
    });
    connect(ui->le_min, &QLineEdit::editingFinished,this,[=](){
        int changvalue = ui->le_min->text().toInt()*100;
        int thisvalue = ui->sb_set->text().toDouble()*100;
        ui->hs_set->setMinimum(changvalue);
        ui->hs_set->setValue(thisvalue);
    });
}

void MainWindows::SerialSendData(const char *data , const int DataLen =1)
{
    static unsigned long sendCount = 0;
    if(serialPort == nullptr){
        qDebug()<<"SerialPort error";
        return;
    }

    if(serialPort->isOpen()){
        /*发送数据*/
        serialPort->write(data,DataLen);
        sendCount+=DataLen;
    }else{
        qDebug()<<"SerialPort error";
    }
}
