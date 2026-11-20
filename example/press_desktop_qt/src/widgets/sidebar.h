#pragma once

#include <QHash>
#include <QWidget>

class RailButton;
class StatusDot;
class QLabel;

// 左侧深色导航栏
class Sidebar : public QWidget
{
    Q_OBJECT
public:
    explicit Sidebar(QWidget *parent = nullptr);

    void setActive(const QString &key);
    void setLocked(const QString &currentKey, bool running);
    void setModeVisible(const QString &key, bool visible);
    void setLinkStatus(bool connected, const QString &detail = QString());
    // 连接设备后用设备名称/型号更新品牌区
    void setBrand(const QString &name, const QString &model);
    // 已连接但实时数据超时：显示“通讯中断”，状态点按离线处理
    void setLinkWarning(const QString &detail = QString());

signals:
    void navRequested(const QString &key);

private:
    QWidget *createBrand();
    QWidget *createFooter();

    QHash<QString, RailButton *> m_items;
    QLabel *m_linkTitle = nullptr;
    QLabel *m_linkDetail = nullptr;
    QLabel *m_brandName = nullptr;
    QLabel *m_brandModel = nullptr;
    StatusDot *m_linkDot = nullptr;
};
