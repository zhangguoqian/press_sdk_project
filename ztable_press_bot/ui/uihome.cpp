//
// Created by 11518 on 2024/7/30.
//

// You may need to build the project (run Qt uic code generator) to get "ui_UiHome.h" resolved

#include <QDebug>
#include <QMenu>
#include "uihome.h"
#include "ui_UiHome.h"
#include "uitranslate.h"
#include "admin/controldialog.h"
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QMetaEnum>
#include <QDateTime>
#include "zgq.h"


UiHome::UiHome(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::UiHome),
    m_MaxStepValue(30)
{
    ui->setupUi(this);
    ui->labelActMpa->setVisible(false);
    ui->labelActMpaValue->setVisible(false);
    ui->labelActMpaUnit->setVisible(false);
    ui->labelMpa->setVisible(false);
    ui->labelMpaValue->setVisible(false);
    ui->labelMpaUnit->setVisible(false);

    ui->pBnStartStop->setStartStopStateText(qtTrId("启动"), qtTrId("停止"));
    ui->pBnStartStop->setState(BUTTON_CAN_START);
    ui->pBnPush->setStartStopStateText(qtTrId("脱模"), qtTrId("停止"));
    ui->pBnPush->setState(BUTTON_CAN_START);

    initConnect();

    ui->tableWidgetStepList->initDBoxVector(m_MaxStepValue);;

    m_DataInterface.setParent(this);
    epMachine->registerDataInterface(&m_DataInterface);

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

void UiHome::initTableWidget()
{

}


UiHome::~UiHome()
{
    delete ui;
}

void UiHome::setRealTimeData(const RealTimeData &realTimeData)
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

void UiHome::setPressData(const PressData& pressData)
{
    ui->spinBoxStepValue->setValue(pressData.m_PStep);
    ui->cBoxTypeValue->setCurrentIndex(pressData.m_Type);
    ui->dBoxAValue->setValue(pressData.m_A);
    ui->dBoxBValue->setValue(pressData.m_B);
    ui->dBoxDiameterValue->setValue(pressData.m_D);
    ui->dBoxOutDValue->setValue(pressData.m_OuterD);
    ui->dBoxInDValue->setValue(pressData.m_InnerD);
    ui->dBoxPushValue->setValue(pressData.m_DemoldValue);
    ui->tableWidgetStepList->setUpPressValue(pressData.m_SetPValue);
    ui->tableWidgetStepList->setDownPressValue(pressData.m_AfterValue);
    ui->tableWidgetStepList->setTimePressValue(pressData.m_KPTime);

    m_PressData = pressData;
}

void UiHome::setReadOnlyData(const ReadOnlyData& readOnlyData)
{
    setMaxStepValue(readOnlyData.m_MaxPStep);
    ui->dBoxPushValue->setRange(.0,readOnlyData.m_MaxPLimit/2);
    ui->pressChartView->setRange(0,readOnlyData.m_MaxPLimit);
    m_ReadOnlyData = readOnlyData;
}

void UiHome::setMaxStepValue(int maxStepValue)
{
    m_MaxStepValue = maxStepValue;
    ui->spinBoxStepValue->setRange(1, maxStepValue);
}


void UiHome::uiToJsonString()
{
    m_PressData.m_AfterValue = ui->tableWidgetStepList->getDownPressValue();
    m_PressData.m_SetPValue = ui->tableWidgetStepList->getUpPressValue();
    m_PressData.m_KPTime = ui->tableWidgetStepList->getTimePressValue();
    m_PressData.m_PStep = ui->spinBoxStepValue->value();
    m_PressData.m_Type = ui->cBoxTypeValue->currentIndex();
    m_PressData.m_A = ui->dBoxAValue->value();
    m_PressData.m_B = ui->dBoxBValue->value();
    m_PressData.m_D = ui->dBoxDiameterValue->value();
    m_PressData.m_OuterD = ui->dBoxOutDValue->value();
    m_PressData.m_InnerD = ui->dBoxInDValue->value();
    m_PressData.m_DemoldValue = ui->dBoxPushValue->value();

}

void UiHome::slotTypeChanged(int value)
{
    if (value == 0)
    {
        ui->tableWidgetStepList->setMpaVisible(false);
    }
    else
    {
        ui->tableWidgetStepList->setMpaVisible(true);
    }
    ui->stackedWidget->setCurrentIndex(value);
}


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
        int info = QMessageBox::warning(nullptr, "提示", "是否关闭当前软件？", QMessageBox::Yes, QMessageBox::No);
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


void UiHome::timerEvent(QTimerEvent* event)
{
    QObject::timerEvent(event);
}

DataInterface::~DataInterface()
{
}

void DataInterface::setParent(void* parent)
{
    mpParent = parent;
}


void DataInterface::onReadOnlyData(int errorCode, uint64_t registerNo, const ReadOnlyData& readOnlyData)
{

    auto uiHome = static_cast<UiHome*>(mpParent);
    uiHome->setReadOnlyData(readOnlyData);
}

void DataInterface::onRealTimeData(int errorCode, const RealTimeData& realTimeData)
{
    auto uiHome = static_cast<UiHome*>(mpParent);
    uiHome->setRealTimeData(realTimeData);
}

void DataInterface::onPressData(int errorCode, const PressData& pressData)
{
    auto uiHome = static_cast<UiHome*>(mpParent);
    uiHome->setPressData(pressData);
}

void DataInterface::onError(uint16_t cmdCode, std::vector<uint8_t> response)
{
    auto uiHome = static_cast<UiHome*>(mpParent);
}

void DataInterface::onCommandError(RetCommand errorCode, uint16_t cmdCode)
{
    if (errorCode != RetCommand::RET_SUCCESS)
    {
        return;
    }
    auto uiHome = static_cast<UiHome*>(mpParent);
}

void DataInterface::onCommandPassWarningError(uint16_t cmdCode)
{
}

void UiHome::slotOpenPortClicked(bool isChecked)
{
    Q_UNUSED(isChecked);
}


void UiHome::slotMachineError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError)
    {
        return;
    }

    QMetaEnum metaEnum = QMetaEnum::fromType<QSerialPort::SerialPortError>();
    QMessageBox::warning(this, "警告", "串口错误,错误信息:" + QString("%1").arg(metaEnum.valueToKey(error)));
    qDebug() << "slotMachineError().";
}

void UiHome::slotStartStopClicked(bool isClicked)
{
    // if (isClicked)
    // {
    //     uiToJsonString();
    //     epMachine->addCommand(SET_PRESS_STEP(m_PressData));
    // }
    //
    // epMachine->addCommand(isClicked?SET_PRESS_START:SET_PRESS_STOP);
}

void UiHome::slotControlDialog(bool isClicked)
{
    Q_UNUSED(isClicked)
    ControlDialog controlDialog;
    controlDialog.exec();
}


void UiHome::slotTimeTimerPress(int step, double setValue, int time, int timer, double press, int liquidPress)
{
    //    qDebug() << time << timer << press;
    if (!ui->widgetInfo->isVisible())
    {
        ui->widgetInfo->setVisible(true);
    }
    ui->labelStepValue->setText(QString::number(step + 1));

    ui->pressChartView->appendPressData(QPointF{time / 1000.0, press});
}

void UiHome::slotComplete()
{
    ui->pBnStartStop->setText("启动");
    ui->widgetInfo->setVisible(false);
}



void UiHome::slotDataListWidget()
{
    //    qDebug() << "slotStackWidget()";
    ui->stackedWidgetMain->setCurrentIndex(1);
}

void UiHome::slotControlWidget()
{
    ui->stackedWidgetMain->setCurrentIndex(0);
}

void UiHome::slotTableWidgetCustomContextMenuRequested(QPoint point)
{

}

void UiHome::updateTableWidget()
{

}

void UiHome::slotFirstClicked()
{
    updateTableWidget();
}

void UiHome::slotTailClicked()
{
    updateTableWidget();
}

void UiHome::slotPreClicked()
{
}

void UiHome::slotNextClicked()
{
}

void UiHome::slotPushClicked(bool isChecked)
{
    // epMachine->addCommand(isChecked?SET_PRESS_START:SET_PRESS_STOP);
}

void UiHome::slotPushValueChanged(double value)
{
}
