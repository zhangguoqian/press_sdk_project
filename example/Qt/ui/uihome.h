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

public slots:
    void slotRealTimeData(int errorCode, const RealTimeData& realTimeData);
    void slotPressData(int errorCode, const PressData& pressData);
    void slotReadOnlyData(int errorCode, const ReadOnlyData& readOnlyData);
    void slotError(uint16_t cmdCode, std::vector<uint8_t> response);

protected:
    void closeEvent(QCloseEvent *event) override;

private:

    Ui::UiHome *ui;
    void initConnect();

    DataInterface m_DataInterface;
    RealTimeData m_RealTimeData{};
    PressData m_PressData{};
    ReadOnlyData m_ReadOnlyData{};

private slots:
    void slotStartStopClicked(bool isChecked);
    void slotPushClicked(bool isChecked);

};


#endif //PRESS_SDK_PROJECT_UIHOME_H