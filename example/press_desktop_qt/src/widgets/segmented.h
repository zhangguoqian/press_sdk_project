#pragma once

#include <QStringList>
#include <QVector>
#include <QWidget>

class QPushButton;

// 分段选择控件（异形 / 圆形 / 矩形、普通 / 速率）
class Segmented : public QWidget
{
    Q_OBJECT
public:
    explicit Segmented(const QStringList &options, QWidget *parent = nullptr);

    void setCurrentIndex(int index);
    int currentIndex() const { return m_current; }
    void setFillWidth(bool fill);

signals:
    void changed(int index);

private:
    QVector<QPushButton *> m_buttons;
    int m_current = 0;
};
