//
// Created by 11518 on 2024/7/30.
//

// You may need to build the project (run Qt uic code generator) to get "ui_UiHome.h" resolved

#include <QDebug>
#include <QMenu>
#include <algorithm>
#include <iterator>
#include "uihome.h"
#include "ui_UiHome.h"
#include "uitranslate.h"
#include "admin/controldialog.h"

#include <QMessageBox>
#include <QMetaEnum>
#include <QDateTime>
#include "zgq.h"


UiHome::UiHome(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::UiHome)
{
    ui->setupUi(this);

    initConnect();

    bool info = false;
    info = epMachine->getReadOnlyData(m_ReadOnlyData);
    if (!info)
    {
        QMessageBox::warning(this,qtTrId("提示"),qtTrId("获取设备参数失败!"));
        return;
    }
    info = epMachine->getPressData(m_PressData);
    if (!info){
        QMessageBox::warning(this,qtTrId("提示"),qtTrId("获取设备配置失败!"));
        return;
    }

    epMachine->registerDataInterface(&m_DataInterface,1);
    connect(&m_DataInterface, SIGNAL(signalError(uint16_t, std::vector<uint8_t>)), this, SLOT(slotError(uint16_t, std::vector<uint8_t>)));
    connect(&m_DataInterface, SIGNAL(signalRealTimeData(int, RealTimeData)), this, SLOT(slotRealTimeData(int, RealTimeData)));
    connect(&m_DataInterface, SIGNAL(signalPressData(int, PressData)), this, SLOT(slotPressData(int, PressData)));
    connect(&m_DataInterface, SIGNAL(signalReadOnlyData(int, uint64_t, ReadOnlyData)), this, SLOT(slotReadOnlyData(int, uint64_t, ReadOnlyData)));
}


void UiHome::initConnect()
{
    // 步骤值改变
    connect(ui->spinBoxStepValue, SIGNAL(valueChanged(int)), ui->tableWidgetStepList,SLOT(slotSetValueChanged(int)));

    /**模具参数**/
    // 形状改变
    connect(ui->cBoxTypeValue, SIGNAL(currentIndexChanged(int)), this, SLOT(slotTypeChanged(int)));
    // 直径值改变
    connect(ui->dBoxDiameterValue, SIGNAL(valueChanged(double)), this, SLOT(slotDiameterValueChanged(double)));
    // A值改变
    connect(ui->dBoxAValue, SIGNAL(valueChanged(double)), this, SLOT(slotAValueChanged(double)));
    // B值改变
    connect(ui->dBoxBValue, SIGNAL(valueChanged(double)), this, SLOT(slotBValueChanged(double)));
    // 外径值改变
    connect(ui->dBoxOutDValue, SIGNAL(valueChanged(double)), this, SLOT(slotOuterDiameterValueChanged(double)));
    // 内径值改变
    connect(ui->dBoxInDValue, SIGNAL(valueChanged(double)), this, SLOT(slotInnerDiameterValueChanged(double)));

    /**操作按钮**/

    // 开始/停止按钮点击
    connect(ui->pBnStartStop, SIGNAL(clicked(bool)), this, SLOT(slotStartStopClicked(bool)));
    // 脱模按钮点击
    connect(ui->pBnPush,SIGNAL(clicked(bool)), this, SLOT(slotPushClicked(bool)));
    // 脱模值改变
    connect(ui->dBoxPushValue,SIGNAL(valueChanged(double)), this, SLOT(slotPushValueChanged(double)));

    /**控制按钮**/
    // 控制按钮点击
    connect(ui->pBnControl, SIGNAL(clicked(bool)), this, SLOT(slotControlDialog(bool)));

}



UiHome::~UiHome()
{
    delete ui;
}

void UiHome::slotRealTimeData(int errorCode, const RealTimeData &realTimeData)
{
    if (m_RealTimeData.m_PTime != realTimeData.m_PTime)
    {
        ui->labelActTimeValue->setText(QString::number(realTimeData.m_PTime));
    }
    if (m_RealTimeData.m_CPStep != realTimeData.m_CPStep)
    {
        ui->labelStepValue->setText(QString::number(realTimeData.m_CPStep));
    }
    if (m_RealTimeData.m_ModelState != realTimeData.m_ModelState)
    {

    }
    if (m_RealTimeData.m_PdChanged != realTimeData.m_PdChanged)
    {

    }
    if (m_RealTimeData.m_PressValue != realTimeData.m_PressValue)
    {
        ui->labelActPressValue->setText(QString::number(realTimeData.m_PressValue));
    }
    if (m_RealTimeData.m_PressState != realTimeData.m_PressState)
    {
        if (realTimeData.m_PressState == 0)
        {
            ui->pBnStartStop->setChecked(false);
            ui->pBnPush->setChecked(false);
            ui->pBnPush->setEnabled(true);
            ui->pBnStartStop->setEnabled(true);
        }else if (realTimeData.m_PressState == 1)
        {
            ui->pBnStartStop->setChecked(true);
            ui->pBnPush->setChecked(false);
            ui->pBnPush->setEnabled(false);
        }else if (realTimeData.m_PressState == 2)
        {
            ui->pBnStartStop->setChecked(false);
            ui->pBnPush->setChecked(false);
            ui->pBnStartStop->setEnabled(false);
        }
    }
    static int i = 0;
    ui->pressChartView->appendPressData(QPointF(i++, realTimeData.m_PressValue));
    ui->labelUpValue->setText(QString::number(m_PressData.m_SetPValue[realTimeData.m_CPStep]));
    ui->labelStepValue->setText(QString::number(realTimeData.m_CPStep));
    m_RealTimeData = realTimeData;

}

void UiHome::slotPressData(int errorCode, const PressData& pressData)
{
    if (errorCode != 0)
    {
        qDebug() << Q_FUNC_INFO << errorCode;
        return;
    }
    ui->spinBoxStepValue->setValue(pressData.m_PStep);
    ui->cBoxTypeValue->setCurrentIndex(pressData.m_Type);
    ui->dBoxAValue->setValue(pressData.m_A);
    ui->dBoxBValue->setValue(pressData.m_B);
    ui->dBoxDiameterValue->setValue(pressData.m_D);
    ui->dBoxOutDValue->setValue(pressData.m_OuterD);
    ui->dBoxInDValue->setValue(pressData.m_InnerD);
    ui->dBoxPushValue->setValue(pressData.m_DemoldValue);

    std::vector<float> upVec(std::begin(pressData.m_SetPValue), std::end(pressData.m_SetPValue));
    ui->tableWidgetStepList->setUpPressValue(upVec);

    std::vector<float> downVec(std::begin(pressData.m_AfterValue), std::end(pressData.m_AfterValue));
    ui->tableWidgetStepList->setDownPressValue(downVec);

    std::vector<uint32_t> timeVec(std::begin(pressData.m_KPTime), std::end(pressData.m_KPTime));
    ui->tableWidgetStepList->setTimePressValue(timeVec);

    m_PressData = pressData;
}

void UiHome::slotReadOnlyData(int errorCode, const ReadOnlyData& readOnlyData)
{
    ui->dBoxPushValue->setRange(.0,readOnlyData.m_MaxPLimit/2);
    ui->pressChartView->setRange(0,readOnlyData.m_MaxPLimit);
    m_ReadOnlyData = readOnlyData;
}

void UiHome::slotError(uint16_t cmdCode, std::vector<uint8_t> response)
{
}



// void UiHome::uiToJsonString()
// {
//     auto afterVec = ui->tableWidgetStepList->getDownPressValue();
//     std::copy(afterVec.begin(), afterVec.end(), std::begin(m_PressData.m_AfterValue));
//
//     auto setVec = ui->tableWidgetStepList->getUpPressValue();
//     std::copy(setVec.begin(), setVec.end(), std::begin(m_PressData.m_SetPValue));
//
//     auto timeVec = ui->tableWidgetStepList->getTimePressValue();
//     std::copy(timeVec.begin(), timeVec.end(), std::begin(m_PressData.m_KPTime));
//
//     m_PressData.m_PStep = ui->spinBoxStepValue->value();
//     m_PressData.m_Type = ui->cBoxTypeValue->currentIndex();
//     m_PressData.m_A = ui->dBoxAValue->value();
//     m_PressData.m_B = ui->dBoxBValue->value();
//     m_PressData.m_D = ui->dBoxDiameterValue->value();
//     m_PressData.m_OuterD = ui->dBoxOutDValue->value();
//     m_PressData.m_InnerD = ui->dBoxInDValue->value();
//     m_PressData.m_DemoldValue = ui->dBoxPushValue->value();
// }



void UiHome::closeEvent(QCloseEvent* event)
{
    QWidget::closeEvent(event);
    if (ui->stackedWidgetMain->currentIndex() == 1)
    {
        ui->stackedWidgetMain->setCurrentIndex(0);
        event->ignore();
    }
    else
    {
        int info = QMessageBox::warning(nullptr, qtTrId("提示"), qtTrId("是否关闭当前软件？"), QMessageBox::Yes, QMessageBox::No);
        if (info == QMessageBox::Yes)
        {
            event->accept();
        }
        else
        {
            event->ignore();
        }
    }
}
