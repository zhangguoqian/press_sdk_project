//
// Created by 11518 on 2024/8/20.
//

// You may need to build the project (run Qt uic code generator) to get "ui_controldialog.h" resolved
#include "controldialog.h"
#include <QDateTime>
#include <QtZlib/zlib.h>
#include "ui_controldialog.h"
#include "zgq.h"

int decompressData(uint8_t* dst, ulong* dstLen,const uint8_t* src, ulong srcLen) {
    return uncompress(dst, dstLen, src, srcLen);
}

ControlDialog::ControlDialog(QWidget* parent) :
    QDialog(parent), ui(new Ui::ControlDialog)
{
    ui->setupUi(this);
    connect(ui->pBnGetRodData,SIGNAL(clicked()), this,SLOT(slotGetRodData()));
    connect(ui->pBnGetPdData,SIGNAL(clicked()), this,SLOT(slotGetPdData()));
    connect(ui->pBnGetRtData,SIGNAL(clicked()), this,SLOT(slotGetRtData()));
    connect(ui->pBnRun,SIGNAL(clicked()), this,SLOT(slotRun()));
    connect(ui->pBnStop,SIGNAL(clicked()), this,SLOT(slotStop()));
    // connect(ui->pBnPress,SIGNAL(clicked(bool)), this,SLOT(slotPressing(bool)));
    // connect(ui->pBnDemold,SIGNAL(clicked(bool)), this,SLOT(slotDemolding(bool)));
    // connect(ui->pBnSetPdData,SIGNAL(clicked()), this,SLOT(slotSetPressData()));
}

ControlDialog::~ControlDialog()
{
    delete ui;
}

void ControlDialog::_show_text_log(const QString &data)
{
    ui->textEdit->append(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    ui->textEdit->append(data);
}

void ControlDialog::_show_error_log(const char* error, int info)
{
    ui->textEdit->append(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    ui->textEdit->append(QString(" Error: %1").arg(error).arg(info));
}

void ControlDialog::slotGetRodData()
{
    if (epMachine->isRunning())
    {
        ReadOnlyData readOnlyData;
        if (epMachine->getReadOnlyData(readOnlyData))
        {
            qDebug() << readOnlyData;
            _show_text_log(QString::fromStdString(toJsonString(readOnlyData)));
        }else
        {
            _show_error_log(epMachine->getLastErrorInfo(), 0);
        }
    }
}

void ControlDialog::slotGetPdData()
{
    PressData pressData;
    if (epMachine->getPressData(pressData))
    {
        qDebug() << pressData;
        _show_text_log(QString::fromStdString(toJsonString(pressData)));
    }else
    {
        _show_error_log(epMachine->getLastErrorInfo(), 0);
    }
}

void ControlDialog::slotGetRtData()
{

    RealTimeData realTimeData;
    if (epMachine->getRealTimeData(realTimeData))
    {
        qDebug() << realTimeData;
        _show_text_log(QString::fromStdString(toJsonString(realTimeData)));
    }else
    {
        _show_error_log(epMachine->getLastErrorInfo(), 0);
    }

}

void ControlDialog::slotRun()
{
    if (epMachine->isConnected())
    {
        if (!epMachine->isRunning())
        {
            epMachine->run();
        }
    }
}

void ControlDialog::slotStop()
{
    if (epMachine->isConnected())
    {
        if (epMachine->isRunning())
        {
            epMachine->stop();
        }
    }
}

void ControlDialog::slotPressing(bool isPressing)
{
    if (isPressing)
    {
        bool ret = epMachine->setPressing(isPressing);
        if (ret)
        {
            _show_text_log(QString("start press success"));
        }else
        {
            _show_error_log(epMachine->getLastErrorInfo(), 0);
        }
    }else
    {
        bool ret = epMachine->setPressing(isPressing);
        if (ret)
        {
            _show_text_log("stop press success");
        }else
        {
            _show_error_log(epMachine->getLastErrorInfo(), 0);
        }
    }
}

void ControlDialog::slotDemolding(bool isDemolding)
{
    if (isDemolding)
    {
        bool ret = epMachine->setDemolding(isDemolding);
        if (ret)
        {
            _show_text_log(QString("start demolding success"));
        }else
        {
            _show_error_log(epMachine->getLastErrorInfo(), 0);
        }
    }else
    {
        bool ret = epMachine->setDemolding(isDemolding);
        if (ret)
        {
            _show_text_log("stop demolding success");
        }else
        {
            _show_error_log(epMachine->getLastErrorInfo(), 0);
        }
    }
}

void ControlDialog::slotSetPressData()
{
    PressData pressData;
    if (epMachine->getPressData(pressData))
    {
        qDebug() << pressData;
        _show_text_log(QString::fromStdString(toJsonString(pressData)));
    }else
    {
        _show_error_log(epMachine->getLastErrorInfo(), 0);
    }
    pressData.m_PStep = 1;
    if (epMachine->setPressData(pressData))
    {
        _show_text_log("set press data success");
    }else
    {
        _show_error_log(epMachine->getLastErrorInfo(), 0);
    }
}