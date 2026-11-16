//
// Created by 11518 on 2024/7/30.
//

#ifndef PRESS_SDK_PROJECT_UIHOME_H
#define PRESS_SDK_PROJECT_UIHOME_H

#include <QMainWindow>
#include <QVector>

#include "press.hpp"
#include "datainterface.h"

class QLabel;
class QPushButton;
class QToolButton;
class QTableWidget;
class QStackedWidget;
class QSpinBox;
class QDoubleSpinBox;
class QComboBox;
class QCheckBox;
class QSlider;
class StepTableWidget;
class PressChartView;

//! 主界面：按 1024x600 触摸屏 UI 实现，支持单独模式 / 合作模式
//! Main window matching the 1024x600 touch-panel UI (separate / cooperation mode)
class UiHome : public QMainWindow
{
    Q_OBJECT

public:
    explicit UiHome(QWidget* parent = nullptr);
    ~UiHome() override;

public slots:
    void slotRealTimeData(int errorCode, const RealTimeData& realTimeData);
    void slotPressData(int errorCode, const PressData& pressData);
    void slotReadOnlyData(int errorCode, uint64_t registerNo, const ReadOnlyData& readOnlyData);
    void slotError(uint16_t cmdCode, std::vector<uint8_t> response);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void slotClockTick();

    // 底部导航 / bottom navigation
    void slotToggleMode();
    void slotShowData();
    void slotShowChart();
    void slotShowProcess();
    void slotShowSystem();

    // 操作按钮 / action buttons
    void slotStartPress(bool isChecked);
    void slotStartHeat(bool isChecked);
    void slotStartCooperate(bool isChecked);
    void slotDemold(bool isChecked);

    // 流程设置写回 / process setting write-back
    void slotStepCountChanged(int value);
    void slotMoldTypeChanged(int value);
    void slotDiameterChanged(double value);
    void slotAChanged(double value);
    void slotBChanged(double value);
    void slotOuterDChanged(double value);
    void slotInnerDChanged(double value);
    void slotDemoldValueChanged(double value);
    void slotCellValueChanged(int row, int column, double value);
    void slotControlDialog();

    // 系统设置 / system settings
    void slotBrightnessReleased();
    void slotSoundToggled(bool enabled);
    void slotLanguageChanged(int index);
    void slotReadDeviceTime();
    void slotWriteDeviceTime();

private:
    void buildUi();
    QWidget* buildTopBar();
    QWidget* buildSeparatePage();
    QWidget* buildCooperatePage();
    QWidget* buildChartPage();
    QWidget* buildProcessPage();
    QWidget* buildSystemPage();
    QWidget* buildDataPage();
    QWidget* buildBottomBar();

    QWidget* createInfoRow(const QString& caption, QLabel** value, const QString& initial,
                           int labelWidth = 100, int height = 62);
    QWidget* createDualRow(const QString& caption, QLabel** value1, QLabel** value2,
                           const QString& initial1, const QString& initial2,
                           int labelWidth = 100, int height = 62);
    QTableWidget* createTable(const QStringList& headers);
    QToolButton* createNavButton(const QString& text, const QString& name);

    void initConnect();
    void applyStyle();
    void loadDeviceData();
    void refreshStepTables();
    void updateBottomBar(bool separateMode);
    void setActiveNav(QToolButton* button, bool active);
    void showPage(int index);
    static void repolish(QWidget* widget);
    QString fmtMs(uint32_t ms) const;

    QStackedWidget* m_pStackMain = nullptr;

    // 顶部 / top bar
    QLabel* m_pLabelDateTime = nullptr;
    QLabel* m_pLabelVersion = nullptr;

    // 单独模式 / separate mode
    QLabel* m_pLabelPressStepHdr = nullptr;
    QLabel* m_pLabelSetPress = nullptr;
    QLabel* m_pLabelAfterPress = nullptr;
    QLabel* m_pLabelRealPress = nullptr;
    QLabel* m_pLabelMoldPress = nullptr;
    QLabel* m_pLabelPressTimer = nullptr;
    QTableWidget* m_pTablePressSteps = nullptr;
    QLabel* m_pLabelTempStepHdr = nullptr;
    QLabel* m_pLabelSetTemp = nullptr;
    QLabel* m_pLabelUpTemp = nullptr;
    QLabel* m_pLabelDownTemp = nullptr;
    QLabel* m_pLabelTempTimer = nullptr;
    QTableWidget* m_pTableTempSteps = nullptr;

    // 合作模式 / cooperation mode
    QLabel* m_pLabelMixStepHdr = nullptr;
    QLabel* m_pLabelSetPressA = nullptr;
    QLabel* m_pLabelSetPressB = nullptr;
    QLabel* m_pLabelRealPressC = nullptr;
    QLabel* m_pLabelMoldPressC = nullptr;
    QLabel* m_pLabelUpTempA = nullptr;
    QLabel* m_pLabelUpTempB = nullptr;
    QLabel* m_pLabelDownTempA = nullptr;
    QLabel* m_pLabelDownTempB = nullptr;
    QLabel* m_pLabelMixTimer = nullptr;
    QTableWidget* m_pTableMixSteps = nullptr;

    // 列表/曲线 / chart
    PressChartView* m_pChartView = nullptr;

    // 流程设置 / process settings
    QSpinBox* m_pSpinStepCount = nullptr;
    QComboBox* m_pComboMoldType = nullptr;
    QStackedWidget* m_pStackMold = nullptr;
    QDoubleSpinBox* m_pDBoxDiameter = nullptr;
    QDoubleSpinBox* m_pDBoxA = nullptr;
    QDoubleSpinBox* m_pDBoxB = nullptr;
    QDoubleSpinBox* m_pDBoxOuterD = nullptr;
    QDoubleSpinBox* m_pDBoxInnerD = nullptr;
    StepTableWidget* m_pTableStepSettings = nullptr;
    QDoubleSpinBox* m_pDBoxDemold = nullptr;
    QPushButton* m_pBtnDemold = nullptr;
    QPushButton* m_pBtnControl = nullptr;

    // 系统设置 / system settings
    QSlider* m_pSliderBrightness = nullptr;
    QLabel* m_pLabelBrightnessValue = nullptr;
    QCheckBox* m_pChkSound = nullptr;
    QComboBox* m_pComboLanguage = nullptr;
    QLabel* m_pLabelDeviceTime = nullptr;

    // 底部导航 / bottom navigation
    QToolButton* m_pBtnStartPress = nullptr;
    QToolButton* m_pBtnMode = nullptr;
    QToolButton* m_pBtnData = nullptr;
    QToolButton* m_pBtnChart = nullptr;
    QToolButton* m_pBtnProcess = nullptr;
    QToolButton* m_pBtnSystem = nullptr;
    QToolButton* m_pBtnStartHeat = nullptr;
    QToolButton* m_pBtnStartCooperate = nullptr;

    // 数据 / data
    DataInterface m_DataInterface;
    RealTimeData m_RealTimeData{};
    PressData m_PressData{};
    ReadOnlyData m_ReadOnlyData{};
    QVector<double> m_TempSetPoints;
    QVector<uint32_t> m_TempKTimes;
    int m_TempStepCount = 0;
    quint64 m_ChartIndex = 0;
    bool m_SeparateMode = true;

    //! 构造期间设备数据回填 UI 会触发 valueChanged，
    //! 用该标志抑制误写回；构造完成置 true
    bool m_UiReady = false;
};

#endif //PRESS_SDK_PROJECT_UIHOME_H
