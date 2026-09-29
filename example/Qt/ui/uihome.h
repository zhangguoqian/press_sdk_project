//
// Created by 11518 on 2024/7/30.
//

#ifndef ZTABLE_PRESS_BOT_PROJECT_UIHOME_H
#define ZTABLE_PRESS_BOT_PROJECT_UIHOME_H

#include <QMainWindow>
#include <QTableWidgetItem>
#include "machinetype.h"

class DataInterface: public MachineDataInterface
{
public:
    ~DataInterface() override;
    void setParent(void* parent);
    void onReadOnlyData(int errorCode,uint64_t registerNo, const ReadOnlyData& readOnlyData) override;
    void onRealTimeData(int errorCode, const RealTimeData& realTimeData) override;
    void onPressData(int errorCode, const PressData& pressData) override;
    void onError(uint16_t cmdCode,std::vector<uint8_t> response) override;
private:
    void *mpParent = nullptr;
};

QT_BEGIN_NAMESPACE
namespace Ui { class UiHome; }
QT_END_NAMESPACE

class UiHome : public QMainWindow {
Q_OBJECT

signals:
    void signalGetComPortList();
public:
    explicit UiHome(QWidget *parent = nullptr);
    ~UiHome() override;
    void setRealTimeData(const RealTimeData& realTimeData);
    void setPressData(const PressData& pressData);
    void setReadOnlyData(const ReadOnlyData& readOnlyData);
    void setMaxStepValue(int mMaxStepValue);


protected:
    void closeEvent(QCloseEvent *event) override;

    void timerEvent(QTimerEvent *event) override;

private:

    Ui::UiHome *ui;
    void initConnect();

    DataInterface m_DataInterface;
    RealTimeData m_RealTimeData{};
    PressData m_PressData{};
    ReadOnlyData m_ReadOnlyData{};

    int m_MaxStepValue;

    void initTableWidget();
    void updateTableWidget();
    QVector<QTableWidgetItem*> mpNoItemList;
    QVector<QTableWidgetItem*> mpDateTimeItemList;
    QVector<QTableWidgetItem*> mpTypeItemList;
    QVector<QTableWidgetItem*> mpSizeItemList;
    QVector<QTableWidgetItem*> mpMaxPressItemList;

    void uiToJsonString();

  private slots:
    //    void slotStepValueChanged(int value);
    void slotTypeChanged(int value);
    void slotOpenPortClicked(bool isChecked);
    void slotStartStopClicked(bool isClicked);
    void slotPushClicked(bool isChecked);
    void slotPushValueChanged(double value);
    void slotControlDialog(bool isClicked);

    void slotTimeTimerPress(int step,double setValue,int time, int timer,double press,int liquidPress);
    void slotComplete();
    void slotDataListWidget();
    void slotControlWidget();

    /**tableWidget**/
    void slotTableWidgetCustomContextMenuRequested(QPoint point);
    void slotFirstClicked();
    void slotTailClicked();
    void slotPreClicked();
    void slotNextClicked();
};


#endif //ZTABLE_PRESS_BOT_PROJECT_UIHOME_H