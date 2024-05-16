#ifndef WIDGET_H
#define WIDGET_H

#include <QFile>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget(){};

private:
    void startWindowInit(void);
};
#endif // WIDGET_H
