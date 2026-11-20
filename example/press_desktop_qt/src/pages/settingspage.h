#pragma once

#include <QPair>
#include <QVector>
#include <QWidget>

class MachineState;
class PressService;
struct PressReadOnlyData;
struct DeviceVersionInfo;
class QLabel;
class QLineEdit;
class QPushButton;

// 系统设置：关于本机
class SettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPage(MachineState *state, PressService *service, QWidget *parent = nullptr);

private:
    QWidget *buildTabCard();
    QWidget *buildLinkCard();
    QWidget *buildHeroCard();
    QWidget *buildAboutCard();
    QWidget *buildAboutList(const QVector<QPair<QString, QString>> &rows,
                            const QVector<QLabel **> &outs);
    void updateLinkUi(bool connected);
    void connectOrDisconnect();
    void applyReadOnlyData(const PressReadOnlyData &data);
    void applyVersionInfo(const DeviceVersionInfo &info);

    MachineState *m_state = nullptr;
    PressService *m_service = nullptr;
    QLineEdit *m_endpoint = nullptr;
    QLabel *m_linkState = nullptr;
    QPushButton *m_linkButton = nullptr;
    QLabel *m_heroLinkValue = nullptr;
    QLabel *m_heroTitle = nullptr;   // 品牌区标题，随设备名称自动更新
    QLabel *m_heroSub = nullptr;     // 品牌区副标题，随设备型号自动更新

    // “关于本机”中由 ReadOnlyData 填充的值控件
    QLabel *m_devName = nullptr;
    QLabel *m_devType = nullptr;
    QLabel *m_devSerial = nullptr;
    QLabel *m_devDiameter = nullptr;
    QLabel *m_devMaxP = nullptr;
    QLabel *m_devMinP = nullptr;
    QLabel *m_devMaxT = nullptr;
    QLabel *m_devMinT = nullptr;
    QLabel *m_devBoost = nullptr;
    QLabel *m_devPrecision = nullptr;
    QLabel *m_devPStep = nullptr;
    QLabel *m_devTStep = nullptr;
    // 固件版本信息（连接握手时缓存）
    QLabel *m_devAppType = nullptr;
    QLabel *m_devVersion = nullptr;
};
