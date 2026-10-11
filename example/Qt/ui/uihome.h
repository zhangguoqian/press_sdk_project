//
// Created by 11518 on 2024/7/30.
//

#ifndef PRESS_SDK_PROJECT_UIHOME_H
#define PRESS_SDK_PROJECT_UIHOME_H

#include <QMainWindow>
#include <QTableWidgetItem>
#include "press.hpp"
#include "datainterface.h"

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
    void setMaxStepValue(int mMaxStepValue);
public slots:
    void slotRealTimeData(int errorCode, const RealTimeData& realTimeData);
    void slotPressData(int errorCode, const PressData& pressData);
    void slotReadOnlyData(int errorCode, const ReadOnlyData& readOnlyData);
    void slotError(uint16_t cmdCode, std::vector<uint8_t> response);




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


#endif //PRESS_SDK_PROJECT_UIHOME_H