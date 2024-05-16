#include "MainWindows.h"
#include "ui_MainWindows.h"

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
    //清除接收框消息按钮信号和槽
    mqttClient = new QMqttClient;
    connect( ui->connect_mqtt,&QPushButton::clicked,this,[&](){
        if(ui->connect_mqtt->text() == "请求连接"){
            mqttClient->setHostname(ui->mqtt_ip->text());
            mqttClient->setPort(ui->mqtt_port->text().toInt());
            mqttClient->setClientId(ui->mqtt_id->text());
            mqttClient->setUsername(ui->mqtt_user->text());
            mqttClient->setPassword(ui->mqtt_password->text());
            ui->RxDataTextEdit->clear();
            mqttClient->connectToHost();
            ui->connect_mqtt->setText("断开连接");
        }else{
            mqttClient->disconnectFromHost();
            ui->connect_mqtt->setText("请求连接");
        }
    });

    connect(mqttClient, &QMqttClient::stateChanged, this, [=](){
        auto state = mqttClient->state();
        if(state == QMqttClient::Connecting){
            ui->RxDataTextEdit->append("正在连接服务器");
            qDebug()<<"正在连接服务器";
        }else if(state == QMqttClient::Connected){
            mqttClient->subscribe(QString("trage"));
            mqttClient->subscribe(QString("measure"));
            ui->RxDataTextEdit->append("mqtt服务器连接成功");
            qDebug()<<"mqtt服务器连接成功";
        }else if(state == QMqttClient::Disconnected){
            ui->RxDataTextEdit->append("断开mqtt服务器连接");
            qDebug()<<"断开mqtt服务器连接";
        }
    });

    connect(mqttClient, &QMqttClient::messageReceived, this, [=](const QByteArray &message, const QMqttTopicName &topic) {
        qDebug()<< QLatin1String(" Received Topic: ")
                        + topic.name()
                        + QLatin1String(" Message: ")
                        + message
                        + QLatin1Char('\n');
        ui->RxDataTextEdit->append("Received Topic:" + topic.name() + ", Message:" + message);
        emit DrawMqttData(topic.name(),message.toFloat());
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
        if(mqttClient->state() == QMqttClient::Connected){
            mqttClient->publish(QMqttTopicName("set_temp"), ui->sb_set->text().toUtf8(), 0, 1);
        }
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
        if(mqttClient->state() == QMqttClient::Connected){
            mqttClient->publish(QMqttTopicName("set_temp"), ui->sb_set->text().toUtf8(), 0, 1);
        }
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
