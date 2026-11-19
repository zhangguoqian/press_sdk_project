#pragma once

#include <QHash>
#include <QWidget>

class RailButton;

// 左侧深色导航栏
class Sidebar : public QWidget
{
    Q_OBJECT
public:
    explicit Sidebar(QWidget *parent = nullptr);

    void setActive(const QString &key);
    void setLocked(const QString &currentKey, bool running);

signals:
    void navRequested(const QString &key);

private:
    QWidget *createBrand();
    QWidget *createFooter();

    QHash<QString, RailButton *> m_items;
};
