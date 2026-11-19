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
    void setLinkStatus(bool connected, const QString &detail = QString());

signals:
    void navRequested(const QString &key);

private:
    QWidget *createBrand();
    QWidget *createFooter();

    QHash<QString, RailButton *> m_items;
    QLabel *m_linkTitle = nullptr;
    QLabel *m_linkDetail = nullptr;
    StatusDot *m_linkDot = nullptr;
};
