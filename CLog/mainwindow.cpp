#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "clog.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    for (int i = 0; i < 10000; i++)
    {
        log(LOG_LEVEL_DEBUG, "%s,%4d", "你好", 1);
        log(LOG_LEVEL_ERROR, "%4d,%4d", 4, 1);
    }

}



ThreadTest::ThreadTest(QObject *parent) :
    QThread(parent)
{

}


void ThreadTest::run(void)
{
    for(int i = 0; i < 5000; i++)
    {
        log(LOG_LEVEL_DEBUG, "%4d",  i + 1);
        msleep(1);
    }
}
