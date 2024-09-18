//
// Created by 11518 on 2024/7/30.
//

// You may need to build the project (run Qt uic code generator) to get "ui_UiHome.h" resolved

#include <QDebug>
#include <QMenu>
#include "uihome.h"
#include "ui_UiHome.h"
#include "uitranslate.h"
#include "pcrhead.h"
#include "admin/controldialog.h"
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QMetaEnum>
#include <QDateTime>

UiHome::UiHome(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::UiHome),
        m_MaxStepValue(DEFAULT_MAX_STEP){
    ui->setupUi(this);
    this->setWindowTitle(UiTr::AppName + VersionNumber.toString());
    this->resize(APP::AppUiSize());

    // ui->statusbar->showMessage("message");

    initConnect();
    setPortWidgetEnabled(false);
    ui->widgetInfo->setVisible(false);
    ui->stackedWidgetMain->setCurrentIndex(0);

    setPBnVisible(false);
//    startTimer(1000);

    {
        setMaxStepValue(m_MaxStepValue);
    }
    ui->cBoxTypeValue->setCurrentIndex(epActualPressData->s_Type);
    ui->dBoxDiameterValue->setValue(epActualPressData->s_D);
    ui->dBoxAValue->setValue(epActualPressData->s_A);
    ui->dBoxBValue->setValue(epActualPressData->s_B);
    ui->stackedWidget->setCurrentIndex(epActualPressData->s_Type);


    ui->spinBoxStepValue->setValue(epActualPressData->s_Step);
    ui->tableWidgetStepList->setStepValueCount(ui->spinBoxStepValue->value());
    setTypeValue(ui->cBoxTypeValue->currentIndex());

    initTableWidget();
}


void UiHome:: initConnect() {
    QMetaObject::Connection stepConnect =
            connect(ui->spinBoxStepValue,SIGNAL(valueChanged(int)),ui->tableWidgetStepList, SLOT(slotSetValueChanged(int)));
    connect(ui->cBoxTypeValue,SIGNAL(currentIndexChanged(int)),this, SLOT(slotTypeChanged(int)));

    connect(ui->dBoxDiameterValue,QOverload<double>::of(&QDoubleSpinBox::valueChanged),this,[=](double value){
        epActualPressData->s_D = value;
        ui->tableWidgetStepList->updateData();
    });
    connect(ui->dBoxAValue,QOverload<double>::of(&QDoubleSpinBox::valueChanged),this,[=](double value){
        epActualPressData->s_A = value;
        ui->tableWidgetStepList->updateData();
    });
    connect(ui->dBoxBValue,QOverload<double>::of(&QDoubleSpinBox::valueChanged),this,[=](double value){
        epActualPressData->s_B = value;
        ui->tableWidgetStepList->updateData();
    });
//
    connect(ui->cBoxTypeValue,QOverload<int>::of(&QComboBox::currentIndexChanged),[=](int value){
        epActualPressData->s_Type = value;
        ui->tableWidgetStepList->updateData();
    });

    connect(ui->pBnUpdateTty, SIGNAL(clicked(bool)),this, SLOT(slotOpenPortClicked(bool)));

    connect(ui->pBnStartStop, SIGNAL(clicked(bool)),this, SLOT(slotStartStopClicked(bool)));

    connect(ui->pBnControl, SIGNAL(clicked(bool)),this,SLOT(slotControlDialog(bool)));

    connect(ui->pBnData, SIGNAL(pressed()),this,SLOT(slotDataListWidget()));

//    connect(ui->pBnData, SIGNAL(pressed()),this,SLOT(slotControlWidget()));

    connect(epMachine, SIGNAL(signalMachineError(QSerialPort::SerialPortError)),this,SLOT(slotMachineError(QSerialPort::SerialPortError)));

    connect(this, SIGNAL(signalGetComPortList()),epMachine,SLOT(slotGetComPortList()));
    connect(epMachine, SIGNAL(signalPortName(QString)),this,SLOT(slotPortName(QString)));
    connect(epMachine, SIGNAL(signalTimeTimerPress(int,double,int,int,double,int)),this,SLOT(slotTimeTimerPress(int,double,int,int,double,int)));
    connect(epMachine, SIGNAL(signalComplete()),this,SLOT(slotComplete()));

}

void UiHome::initTableWidget(){
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
    ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->setEditTriggers(QTableWidget::NoEditTriggers);
    ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableWidget,SIGNAL(customContextMenuRequested(QPoint)), SLOT(slotTableWidgetCustomContextMenuRequested(QPoint)));

    m_CurrentPage = 1;
    int row = ui->tableWidget->rowCount();
    m_PageSize = row;
    mpNoItemList.resize(row);
    mpDateTimeItemList.resize(row);
    mpTypeItemList.resize(row);
    mpSizeItemList.resize(row);
    mpMaxPressItemList.resize(row);

    for (int i = 0; i < row; ++i) {
        mpNoItemList[i] = new QTableWidgetItem();
        ui->tableWidget->setItem(i,0,mpNoItemList[i]);
        mpNoItemList[i]->setTextAlignment(Qt::AlignCenter);

        mpDateTimeItemList[i] = new QTableWidgetItem();
        ui->tableWidget->setItem(i,1,mpDateTimeItemList[i]);
        mpDateTimeItemList[i]->setTextAlignment(Qt::AlignCenter);

        mpTypeItemList[i] = new QTableWidgetItem();
        ui->tableWidget->setItem(i,2,mpTypeItemList[i]);
        mpTypeItemList[i]->setTextAlignment(Qt::AlignCenter);

        mpSizeItemList[i] = new QTableWidgetItem();
        ui->tableWidget->setItem(i,3,mpSizeItemList[i]);
        mpSizeItemList[i]->setTextAlignment(Qt::AlignCenter);

        mpMaxPressItemList[i] = new QTableWidgetItem();
        ui->tableWidget->setItem(i,4,mpMaxPressItemList[i]);
        mpMaxPressItemList[i]->setTextAlignment(Qt::AlignCenter);
    }
    updateTableWidget();
//    bool sqlInfo = epDataControl->selectDataResultCount(m_SumDataResultCount);
//    if(sqlInfo){
//        if(m_SumDataResultCount==0){
//            return;
//        }
//    }else{
//        return;
//    }
//
//    sqlInfo = epDataControl->selectDataResultList(m_DataResultList,0,row);
//    if(sqlInfo){
//        for (int i = 0; i < m_DataResultList.size(); ++i) {
//            mpNoItemList[i]->setText(QString::number(m_DataResultList[i].s_Id));
//            mpDateTimeItemList[i]->setText(m_DataResultList[i].s_DateTime);
//            ActualPressData actualPressData;
//            bool jsonInfo = actualPressData.openJsonString(m_DataResultList[i].s_JsonData.toLatin1());
//            if(jsonInfo){
//                mpTypeItemList[i]->setText(actualPressData.typeString());
//                mpSizeItemList[i]->setText(actualPressData.sizeString());
//                mpMaxPressItemList[i]->setText(QString::number(actualPressData.maxUpValue(),'f',epReadOnlySetData->s_PressPrecision));
//            }
//        }
//    }

}



UiHome::~UiHome() {
    delete ui;
}

//void UiHome::slotStepValueChanged(int value) {
//    for (int i = 0; i < m_MaxStepValue; ++i) {
//        ui->tableWidgetStepList->setColumnHidden(i,i>=value);
//    }
////    ui->tableWidgetStepList->setColumnCount(value);
//}

void UiHome::setMaxStepValue(int mMaxStepValue) {
    m_MaxStepValue = mMaxStepValue;
//    ui->spinBoxStepValue->setMaximum(mMaxStepValue);
    ui->spinBoxStepValue->setRange(1,mMaxStepValue);

    ui->tableWidgetStepList->setMaxStepValue(m_MaxStepValue);
}

void UiHome::slotTypeChanged(int value) const{
    setTypeValue(value);
}

void UiHome::setTypeValue(int value) const {
    if(value == 0){
        ui->tableWidgetStepList->setMpaVisible(false);
    }else{
        ui->tableWidgetStepList->setMpaVisible(true);
    }
    ui->stackedWidget->setCurrentIndex(value);
}

void UiHome::closeEvent(QCloseEvent *event) {
    QWidget::closeEvent(event);
    if(ui->stackedWidgetMain->currentIndex() == 1){
        ui->stackedWidgetMain->setCurrentIndex(0);
        event->ignore();
    }else{
        int info = QMessageBox::warning(nullptr,"提示","是否关闭当前软件？",QMessageBox::Yes,QMessageBox::No);
        if(info==QMessageBox::Yes){
            event->accept();
        }else{
            event->ignore();
        }
    }
}

QStringList UiHome::getPortList() {
    QStringList portList;
//    ui->cboBoxTty->clear();
    QList<QSerialPortInfo> serialPortList = QSerialPortInfo::availablePorts();
    for (auto &var:serialPortList){
        portList.append(var.portName());
    }
    return portList;
}

void UiHome::timerEvent(QTimerEvent *event) {
//    qDebug() <<ui->cboBoxTty->view()->isVisible();
    if(!ui->pBnUpdateTty->isChecked() && !ui->cboBoxTty->view()->isVisible()){
        QString portName = ui->cboBoxTty->currentText();
        QStringList portNames = getPortList();
        ui->cboBoxTty->clear();
        ui->cboBoxTty->addItems(portNames);
        if(portNames.contains(portName)){
            ui->cboBoxTty->setCurrentText(portName);
        }
    }
    QObject::timerEvent(event);
}

void UiHome::slotOpenPortClicked(bool isChecked) {
    if(epMachine->machineIsRunning()){
        qDebug() << "machine is running.";
    }else{
        emit signalGetComPortList();
    }
}

void UiHome::setPortWidgetEnabled(bool isEnabled) {
    ui->cboBoxTty->setEnabled(!isEnabled);
    ui->pBnAir->setEnabled(isEnabled);
//    ui->pBnStartStop->setEnabled(isEnabled);
}

void UiHome::slotMachineError(QSerialPort::SerialPortError error) {
    if(error==QSerialPort::NoError){
        return;
    }
    epMachine->stopMachine();
    ui->pBnUpdateTty->setChecked(false);
    setPortWidgetEnabled(false);
    QMetaEnum metaEnum = QMetaEnum::fromType<QSerialPort::SerialPortError>();
    QMessageBox::warning(this,"警告","串口错误,错误信息:"+QString("%1").arg(metaEnum.valueToKey(error)));
    qDebug() << "slotMachineError().";
}

void UiHome::slotStartStopClicked(bool isClicked) {
    if(isClicked){
        ui->pBnStartStop->setText("停止");
        epMachine->setPortName(ui->cboBoxTty->currentText());
        epMachine->setPressCmdList(epActualPressData->getPressCmdList());
        epMachine->startMachine();

    }else{
        ui->pBnStartStop->setText("启动");
        epMachine->stopMachine();
        ui->widgetInfo->setVisible(false);
        m_DataResult.s_DateTime = QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss");
        m_DataResult.s_JsonData = epActualPressData->saveJsonString();
        m_DataResult.s_Complete = 0;
        m_DataResult.s_Line = ui->pressChartView->lineToJsonString();
        bool sqlInfo = epDataControl->addDataResult(m_DataResult);
        if(sqlInfo){
            updateTableWidget();
        }
    }
}

void UiHome::slotControlDialog(bool isClicked) {
    ControlDialog controlDialog;
    controlDialog.exec();
}

void UiHome::slotPortName(QString port) {
    ui->cboBoxTty->clear();
    if(port.isEmpty()){
        setPortWidgetEnabled(false);
        QMessageBox::warning(this,"警告","没有发现设备。");
    }else{
        ui->cboBoxTty->addItem(port);
        setPortWidgetEnabled(true);
    }
}

void UiHome::slotTimeTimerPress(int step,double setValue,int time, int timer,double press,int liquidPress) {
//    qDebug() << time << timer << press;
    if(!ui->widgetInfo->isVisible()){
        ui->widgetInfo->setVisible(true);
    }
    ui->labelStepValue->setText(QString::number(step+1));
    ui->labelUpValue->setText(QString::number(setValue,'f',epReadOnlySetData->s_PressPrecision));
    ui->labelActTimeValue->setText(QString::number(timer));
    ui->labelActPressValue->setText(QString::number(press,'f',epReadOnlySetData->s_PressPrecision));
    ui->labelActMpaValue->setText(QString::number(liquidPress/1000.0,'f',epReadOnlySetData->s_PressPrecision));
    double value = APP::tonToTypeMpa(press,ui->stackedWidget->currentIndex(),
                                     ui->dBoxDiameterValue->value(),
                                     ui->dBoxAValue->value(),
                                     ui->dBoxBValue->value()
                                     );
    ui->labelMpaValue->setText(QString::number(value,'f',epReadOnlySetData->s_PressPrecision));
    ui->pressChartView->appendPressData(QPointF{time/1000.0,press});
}

void UiHome::slotComplete() {
    ui->pBnStartStop->setText("启动");
    epMachine->stopMachine();
    ui->widgetInfo->setVisible(false);
}

void UiHome::setPBnVisible(bool isVisible) {
    ui->pBnAir->setVisible(isVisible);
    ui->pBnSet->setVisible(isVisible);
//    ui->pBnData->setVisible(isVisible);
}

void UiHome::slotDataListWidget() {
//    qDebug() << "slotStackWidget()";
    ui->stackedWidgetMain->setCurrentIndex(1);
}

void UiHome::slotControlWidget() {
    ui->stackedWidgetMain->setCurrentIndex(0);
}

void UiHome::slotTableWidgetCustomContextMenuRequested(QPoint point) {
    int row = ui->tableWidget->currentRow();
    if(m_DataResultList.size() > row){
        QMenu *pMenu = new QMenu();
        QAction *pActionOpen = new QAction("查看");
        connect(pActionOpen, &QAction::triggered,this,[=](){

        });
        QAction *pActionUse = new QAction("使用");
        connect(pActionUse, &QAction::triggered,this,[=](){

        });
        QAction *pActionDelete = new QAction("删除");
        connect(pActionDelete, &QAction::triggered,this,[=](){
            int no = mpNoItemList[row]->text().toInt();
            QString dateTime = mpDateTimeItemList[row]->text();
            bool sqlInfo = epDataControl->deleteDataResult(no,dateTime);
            if(sqlInfo){
                updateTableWidget();
            }
        });
        pMenu->addAction(pActionOpen);
        pMenu->addAction(pActionUse);
        pMenu->addAction(pActionDelete);

        pMenu->exec(QCursor::pos());
        delete pActionOpen;
        delete pActionUse;
        delete pActionDelete;
        delete pMenu;
    }
}

void UiHome::updateTableWidget() {
    int row = ui->tableWidget->rowCount();

    bool sqlInfo = epDataControl->selectDataResultCount(m_SumDataResultCount);
    if(sqlInfo){
        if(m_SumDataResultCount==0){
            return;
        }
    }else{
        return;
    }

    sqlInfo = epDataControl->selectDataResultList(m_DataResultList,m_CurrentPage,row);
    int size = m_DataResultList.size();
    if(sqlInfo){
        for (int i = 0; i < size; ++i) {
            mpNoItemList[i]->setText(QString::number(m_DataResultList[i].s_Id));
            mpDateTimeItemList[i]->setText(m_DataResultList[i].s_DateTime);
            ActualPressData actualPressData;
            bool jsonInfo = actualPressData.openJsonString(m_DataResultList[i].s_JsonData.toLatin1());
            if(jsonInfo){
                mpTypeItemList[i]->setText(actualPressData.typeString());
                mpSizeItemList[i]->setText(actualPressData.sizeString());
                mpMaxPressItemList[i]->setText(QString::number(actualPressData.maxUpValue(),'f',epReadOnlySetData->s_PressPrecision));
            }
        }
        for (int i = size; i < row; ++i) {
            mpNoItemList[i]->setText("");
            mpDateTimeItemList[i]->setText("");
            mpTypeItemList[i]->setText("");
            mpSizeItemList[i]->setText("");
            mpMaxPressItemList[i]->setText("");
        }
    }
}

