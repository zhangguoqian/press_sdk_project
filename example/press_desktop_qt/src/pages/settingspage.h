#pragma once

#include <QPair>
#include <QVector>
#include <QWidget>

// 系统设置：关于本机
class SettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPage(QWidget *parent = nullptr);

private:
    QWidget *buildTabCard();
    QWidget *buildHeroCard();
    QWidget *buildAboutCard();
    QWidget *buildAboutList(const QVector<QPair<QString, QString>> &rows);
};
