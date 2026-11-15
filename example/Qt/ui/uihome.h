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
    void slotTypeChanged(int value);
    void slotDiameterValueChanged(double value);
    void slotAValueChanged(double value);
    void slotBValueChanged(double value);
    void slotOuterDiameterValueChanged(double value);
    void slotInnerDiameterValueChanged(double value);
    void slotPushValueChanged(double value);
    void slotControlDialog(bool checked);
    void slotStepValueChanged(int value);
    void slotCellValueChanged(int row, int column, double value);

private:
    //! 构造期间设备数据回填 UI 会触发 valueChanged，
    //! 用该标志抑制误写回；构造完成置 true
    bool m_UiReady = false;

};


#endif //PRESS_SDK_PROJECT_UIHOME_H