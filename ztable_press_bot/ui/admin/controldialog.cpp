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
    connect(ui->pBnAddRodData,SIGNAL(clicked()), this,SLOT(slotAddRodCommand()));
    connect(ui->pBnAddPdData,SIGNAL(clicked()), this,SLOT(slotAddPdCommand()));
}

ControlDialog::~ControlDialog()
{
    delete ui;
}

void ControlDialog::_show_text_log(const std::vector<uint8_t>& data)
{
    ui->textEdit->append(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    QByteArray log = QByteArray::fromRawData((char*)data.data(),data.size());
    if (ui->cBoxIsCompress->isChecked())
    {
        uint8_t *tempData = new uint8_t[data.size() * 10];
        std::vector<uint8_t> srcData = data;
        ulong len = 0;
        int info = decompressData(tempData, &len, reinterpret_cast<const uint8_t*>(log.data()), log.size());
        if (0 == info)
        {
            log = QByteArray::fromRawData((char*)tempData, len);
        }
        delete[] tempData;
    }
    ui->textEdit->append(QString(log.toHex(' ')));
}

void ControlDialog::_show_error_log(const char* error, int info)
{
    ui->textEdit->append(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    ui->textEdit->append(QString(" Error: %1").arg(error).arg(info));
}

void ControlDialog::slotGetRodData()
{
    if (epMachine->isConnected())
    {
        int info = epMachine->runCommand(ui->cBoxIsCompress->isChecked()?GET_ROD_JSON_COMPRESS:GET_ROD_JSON_NORMAL);
        if (0 == info)
        {
            auto response = epMachine->getResponse();
            _show_text_log(response);
        }
        else
        {
            _show_error_log(epMachine->getLastErrorInfo(), info);
        }
    }
}

void ControlDialog::slotGetPdData()
{
    if (epMachine->isConnected())
    {
        int info = epMachine->runCommand(ui->cBoxIsCompress->isChecked()?GET_PD_JSON_COMPRESS:GET_PD_JSON_NORMAL);
        if (0 == info)
        {
            auto response = epMachine->getResponse();
            _show_text_log(response);
        }
        else
        {
            _show_error_log(epMachine->getLastErrorInfo(), info);
        }
    }
}

void ControlDialog::slotGetRtData()
{
    if (epMachine->isConnected())
    {
        int info = epMachine->runCommand(ui->cBoxIsCompress->isChecked()?GET_RT_JSON_COMPRESS:GET_RT_JSON_NORMAL);
        if (0 == info)
        {
            auto response = epMachine->getResponse();
            _show_text_log(response);
        }
        else
        {
            _show_error_log(epMachine->getLastErrorInfo(), info);
        }
    }
}

void ControlDialog::slotRun()
{
    if (epMachine->isConnected())
    {
        if (!epMachine->isRunning())
        {
            epMachine->run();
            // epMachine->registerCommandErrorCallback([](RetCommand errorCode,uint16_t command, const std::vector<uint8_t>& data)
            // {
            //     qDebug() << errorCode << " " << command << " " << data;
            // });
            // epMachine->registerRealTimeDataCallback([](int errorCode,const RealTimeData& realTimeData)
            // {
            //     std::cout << errorCode << realTimeData;
            // });
            // epMachine->registerPressDataCallback([](int errorCode,const PressData& pressData)
            // {
            //     std::cout << errorCode << pressData;
            // });
            // epMachine->registerReadOnlyDataCallback([](int errorCode,const ReadOnlyData& readOnlyData)
            // {
            //     std::cout << errorCode << readOnlyData;
            // });
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

void ControlDialog::slotAddRodCommand()
{
    if (epMachine->isConnected())
    {
        epMachine->addCommand(GET_ROD_JSON_NORMAL);
    }
}

void ControlDialog::slotAddPdCommand()
{
    if (epMachine->isConnected())
    {
        epMachine->addCommand(GET_PD_JSON_NORMAL);
    }
}
