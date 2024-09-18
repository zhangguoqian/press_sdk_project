//
// Created by 11518 on 2024/7/30.
//

#ifndef ZTABLE_PRESS_BOT_PROJECT_UIHOME_H
#define ZTABLE_PRESS_BOT_PROJECT_UIHOME_H

#include <QMainWindow>
#include <QStatusBar>
#include <QSerialPort>
#include <QTableWidgetItem>
#include "sql/datacontrol.h"

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

    void setTypeValue(int value) const;
    Ui::UiHome *ui;
protected:
    void closeEvent(QCloseEvent *event) override;

    void timerEvent(QTimerEvent *event) override;

private:
    void initConnect();
    static QStringList getPortList();
    void setPortWidgetEnabled(bool isEnabled);
    void setPBnVisible(bool isVisible);
    int m_MaxStepValue;
    DataResult m_DataResult;

    void initTableWidget();
    void updateTableWidget();
    QVector<QTableWidgetItem*> mpNoItemList;
    QVector<QTableWidgetItem*> mpDateTimeItemList;
    QVector<QTableWidgetItem*> mpTypeItemList;
    QVector<QTableWidgetItem*> mpSizeItemList;
    QVector<QTableWidgetItem*> mpMaxPressItemList;
    DataResultList m_DataResultList;

    int m_SumDataResultCount;
    int m_CurrentPage;
    int m_SumPage;

  private slots:
    //    void slotStepValueChanged(int value);
    void slotTypeChanged(int value) const;
    void slotOpenPortClicked(bool isChecked);
    void slotMachineError(QSerialPort::SerialPortError error);
    void slotStartStopClicked(bool isClicked);
    void slotControlDialog(bool isClicked);
    void slotPortName(QString port);
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
