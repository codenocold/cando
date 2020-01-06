#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include "api/cando.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    cando_list_handle list;
    cando_handle dev;

    cando_list_malloc(&list);
    cando_list_scan(list);
    uint8_t num;
    cando_list_num(list, &num);
    if(num){
        cando_malloc(list, 0, &dev);
        cando_open(dev);

        uint32_t sw, hw;
        cando_get_dev_info(dev, &sw, &hw);
        qDebug()<<sw<<hw;

        cando_start(dev, CANDO_MODE_NORMAL);

        cando_stop(dev);
    }else{
        qDebug()<<"no device found!";
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

