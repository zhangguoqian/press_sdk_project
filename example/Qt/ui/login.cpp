//
// Created by 11518 on 2026/9/12.
//

// You may need to build the project (run Qt uic code generator) to get "ui_login.h" resolved

#include "login.h"

#include <QMessageBox>

#include "ui_login.h"
#include <QStringList>
#include "zgq.h"
#include "typeprivate.h"

Login::Login(QWidget* parent) :
    QDialog(parent), ui(new Ui::Login)
{
    ui->setupUi(this);
    initUIData();
    connect(ui->pbnConnect,SIGNAL(clicked()),this,SLOT(slotConnect()));
}

Login::~Login()
{
    delete ui;
}

void Login::initUIData()
{
    auto portList = Press::getPortList();
    ui->cboBoxTty->clear();
    for (auto port : portList)
    {
        ui->cboBoxTty->addItem(QString::fromStdString(port));
    }
}

void Login::slotConnect()
{
    QString portName = ui->cboBoxTty->currentText();
    PressPortType portType = SerialPortType;
    if (ui->rBnTty->isChecked())
    {
        portName = ui->cboBoxTty->currentText();
        portType = SerialPortType;
    }
    else if (ui->rBnNetwork->isChecked())
    {
        portType = TcpSocketPortType;
        portName = QString("%1:%2").arg(ui->lEditIp->text()).arg(ui->sBoxPort->value());
    }
    if (epMachine->connect(portName.toStdString().c_str(),portType))
    {
        accept();
    }else
    {
        QMessageBox::critical(this,"Error","Connect to port failed.");
    }
}