#pragma once

#include <QPair>
#include <QVector>
#include <QWidget>

class MachineState;
class PressService;
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
    QWidget *buildAboutList(const QVector<QPair<QString, QString>> &rows);
    void updateLinkUi(bool connected);
    void connectOrDisconnect();

    MachineState *m_state = nullptr;
    PressService *m_service = nullptr;
    QLineEdit *m_endpoint = nullptr;
    QLabel *m_linkState = nullptr;
    QPushButton *m_linkButton = nullptr;
    QLabel *m_heroLinkValue = nullptr;
};
