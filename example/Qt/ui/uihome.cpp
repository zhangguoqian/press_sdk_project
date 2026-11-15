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
#include "ui/admin/controldialog.h"


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
    }else
    {
        ui->pressChartView->setRange(0,m_ReadOnlyData.m_MaxPLimit);
    }
    info = epMachine->getPressData(m_PressData);
    if (!info){
        QMessageBox::warning(this,qtTrId("提示"),qtTrId("获取设备配置失败!"));
        return;
    }else
    {
        ui->cBoxTypeValue->setCurrentIndex(m_PressData.m_Type);
    }

    epMachine->registerDataInterface(&m_DataInterface,1);
    connect(&m_DataInterface, SIGNAL(signalError(uint16_t, std::vector<uint8_t>)), this, SLOT(slotError(uint16_t, std::vector<uint8_t>)));
    connect(&m_DataInterface, SIGNAL(signalRealTimeData(int, RealTimeData)), this, SLOT(slotRealTimeData(int, RealTimeData)));
    connect(&m_DataInterface, SIGNAL(signalPressData(int, PressData)), this, SLOT(slotPressData(int, PressData)));
    connect(&m_DataInterface, SIGNAL(signalReadOnlyData(int, uint64_t, ReadOnlyData)), this, SLOT(slotReadOnlyData(int, uint64_t, ReadOnlyData)));

    // 初始化完成, 允许 UI 编辑写回设备（失败路径提前 return, 保持抑制状态）
    // init done, enable UI write-back (early-return failure paths stay suppressed)
    m_UiReady = true;
}


void UiHome::initConnect()
{
    // 步骤值改变
    connect(ui->spinBoxStepValue, SIGNAL(valueChanged(int)), ui->tableWidgetStepList,SLOT(slotSetValueChanged(int)));
    // 步骤数与单元格编辑写回设备 / step count & cell edits write back to device
    connect(ui->spinBoxStepValue, SIGNAL(valueChanged(int)), this, SLOT(slotStepValueChanged(int)));
    connect(ui->tableWidgetStepList, SIGNAL(signalCellValueChanged(int,int,double)),
            this, SLOT(slotCellValueChanged(int,int,double)));

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

void UiHome::slotStartStopClicked(bool isChecked)
{
    bool info = epMachine->setPressing(isChecked);
    if (info)
    {

    }else
    {
        ui->pBnStartStop->setChecked(!isChecked);
        qDebug() << "start press failed.";
    }
}

void UiHome::slotPushClicked(bool isChecked)
{
    bool info = epMachine->setDemolding(isChecked);
    if (info)
    {

    }else
    {
        ui->pBnPush->setChecked(!isChecked);
        qDebug() << "start demold failed.";
    }
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
    // 回填期间抑制编辑写回, 避免读到的值又被写回设备
    // suppress write-back while refilling UI from device data
    m_UiReady = false;
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
    m_UiReady = true;
}

void UiHome::slotReadOnlyData(int errorCode, const ReadOnlyData& readOnlyData)
{
    ui->dBoxPushValue->setRange(.0,readOnlyData.m_MaxPLimit/2);
    ui->pressChartView->setRange(0,readOnlyData.m_MaxPLimit);
    m_ReadOnlyData = readOnlyData;
}

void UiHome::slotError(uint16_t cmdCode, std::vector<uint8_t> response)
{
    qDebug() << Q_FUNC_INFO << cmdCode << response;
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

// ==================== UI 编辑写回设备 / write-back slots ====================
// 说明: 每个控件改动立即以单项命令写回, 全部使用真机验证过的接口;
//       回填期间 m_UiReady=false, 用户编辑才触发写回

//! 模具类型 → CMDID_MOLD_TYPE / mold type
void UiHome::slotTypeChanged(int value)
{
    if (!m_UiReady)
    {
        return;
    }
    m_PressData.m_Type = value;
    if (!epMachine->setByteValue(CMDID_MOLD_TYPE, static_cast<uint8_t>(value)))
    {
        qDebug() << Q_FUNC_INFO << "setByteValue(MOLD_TYPE) failed:" << epMachine->getLastErrorInfo();
    }
}

//! 圆形直径 → CMDID_CIRCLE_D / circle diameter
void UiHome::slotDiameterValueChanged(double value)
{
    if (!m_UiReady)
    {
        return;
    }
    m_PressData.m_D = static_cast<float>(value);
    if (!epMachine->setFloatValue(CMDID_CIRCLE_D, static_cast<float>(value)))
    {
        qDebug() << Q_FUNC_INFO << "setFloatValue(CIRCLE_D) failed:" << epMachine->getLastErrorInfo();
    }
}

//! 矩形 A → 与 B 合并为 CMDID_RECT_AB 写回 / rectangular A, written together with B
void UiHome::slotAValueChanged(double value)
{
    if (!m_UiReady)
    {
        return;
    }
    m_PressData.m_A = static_cast<float>(value);
    if (!epMachine->setFloatPairValue(CMDID_RECT_AB,
                                      static_cast<float>(ui->dBoxAValue->value()),
                                      static_cast<float>(ui->dBoxBValue->value())))
    {
        qDebug() << Q_FUNC_INFO << "setFloatPairValue(RECT_AB) failed:" << epMachine->getLastErrorInfo();
    }
}

//! 矩形 B → 与 A 合并为 CMDID_RECT_AB 写回 / rectangular B, written together with A
void UiHome::slotBValueChanged(double value)
{
    if (!m_UiReady)
    {
        return;
    }
    m_PressData.m_B = static_cast<float>(value);
    if (!epMachine->setFloatPairValue(CMDID_RECT_AB,
                                      static_cast<float>(ui->dBoxAValue->value()),
                                      static_cast<float>(ui->dBoxBValue->value())))
    {
        qDebug() << Q_FUNC_INFO << "setFloatPairValue(RECT_AB) failed:" << epMachine->getLastErrorInfo();
    }
}

//! 环形外径 → 固件 PD 无对应字段, 仅更新本地缓存 / no firmware field, local cache only
void UiHome::slotOuterDiameterValueChanged(double value)
{
    if (!m_UiReady)
    {
        return;
    }
    // 固件 PD JSON 不携带 OuterD, 设备端无单项命令, 仅同步本地缓存
    m_PressData.m_OuterD = static_cast<float>(value);
}

//! 环形内径 → 固件 PD 无对应字段, 仅更新本地缓存 / no firmware field, local cache only
void UiHome::slotInnerDiameterValueChanged(double value)
{
    if (!m_UiReady)
    {
        return;
    }
    // 固件 PD JSON 不携带 InnerD, 设备端无单项命令, 仅同步本地缓存
    m_PressData.m_InnerD = static_cast<float>(value);
}

//! 脱模压力 → CMDID_DEMOLD_VALUE / demold pressure
void UiHome::slotPushValueChanged(double value)
{
    if (!m_UiReady)
    {
        return;
    }
    m_PressData.m_DemoldValue = static_cast<float>(value);
    if (!epMachine->setFloatValue(CMDID_DEMOLD_VALUE, static_cast<float>(value)))
    {
        qDebug() << Q_FUNC_INFO << "setFloatValue(DEMOLD_VALUE) failed:" << epMachine->getLastErrorInfo();
    }
}

//! 打开控制对话框 / open control dialog
void UiHome::slotControlDialog(bool checked)
{
    Q_UNUSED(checked);
    ControlDialog dlg(this);
    dlg.exec();
}

//! 步骤数 → CMDID_PRESS_STEP / step count
void UiHome::slotStepValueChanged(int value)
{
    if (!m_UiReady)
    {
        return;
    }
    m_PressData.m_PStep = static_cast<uint8_t>(value);
    if (!epMachine->setByteValue(CMDID_PRESS_STEP, static_cast<uint8_t>(value)))
    {
        qDebug() << Q_FUNC_INFO << "setByteValue(PRESS_STEP) failed:" << epMachine->getLastErrorInfo();
    }
}

//! 表格单元格编辑 → 索引类命令写回 / cell edits via indexed commands
//! row: 1=设定压力 2=保压值 3=保压时间(毫秒)
void UiHome::slotCellValueChanged(int row, int column, double value)
{
    if (!m_UiReady)
    {
        return;
    }
    bool ok = false;
    switch (row)
    {
        case 1: // 设定压力 / target pressure
            if (column < MAX_PStep)
            {
                m_PressData.m_SetPValue[column] = static_cast<float>(value);
                ok = epMachine->setIndexedFloat(CMDID_INDEX_PRESS, column, static_cast<float>(value));
            }
            break;
        case 2: // 保压值 / after pressure
            if (column < MAX_PStep)
            {
                m_PressData.m_AfterValue[column] = static_cast<float>(value);
                ok = epMachine->setIndexedFloat(CMDID_INDEX_AFTER, column, static_cast<float>(value));
            }
            break;
        case 3: // 保压时间(毫秒) / keep time (ms)
            if (column < MAX_PStep)
            {
                m_PressData.m_KPTime[column] = static_cast<uint32_t>(value);
                ok = epMachine->setIndexedInt32(CMDID_INDEX_KPTIME, column, static_cast<int32_t>(value));
            }
            break;
        default:
            return;
    }
    if (!ok)
    {
        qDebug() << Q_FUNC_INFO << "indexed write failed, row=" << row
                 << " column=" << column << " err:" << epMachine->getLastErrorInfo();
    }
}
