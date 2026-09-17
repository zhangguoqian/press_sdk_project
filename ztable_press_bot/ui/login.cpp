//
// Created by 11518 on 2026/9/12.
//

// You may need to build the project (run Qt uic code generator) to get "ui_login.h" resolved

#include "login.h"

#include <QMessageBox>

#include "ui_login.h"
#include <QStringList>
#include "zgq.h"
#include "control/typeprivate.h"

Login::Login(QWidget* parent) :
    QDialog(parent), ui(new Ui::Login)
{
    ui->setupUi(this);
    initPortList();
    connect(ui->pbnConnect,SIGNAL(clicked()),this,SLOT(slotConnect()));
}

Login::~Login()
{
    delete ui;
}

void Login::initPortList()
{
    auto portList = Machine::getPortList();
    ui->cboBoxTty->clear();
    for (auto port : portList)
    {
        ui->cboBoxTty->addItem(QString::fromStdString(port));
    }
}

void Login::slotConnect()
{
    std::string portName = ui->cboBoxTty->currentText().toStdString();
    if (epMachine->connect(portName.c_str()))
    {
        if (0==epMachine->runCommand(GET_ROD_JSON_NORMAL))
        {
            accept();
        }
        else
        {
            epMachine->disconnect();
            QMessageBox::critical(this,"Error","Get current state failed.");
        }
    }
    else
    {
        QMessageBox::critical(this,"Error","Connect to port failed.");
    }
}
