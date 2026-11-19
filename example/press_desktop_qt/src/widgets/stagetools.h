#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;

// 卡片头部右侧：阶段数 spinbox + 当前阶段胶囊
class StageTools : public QWidget
{
    Q_OBJECT
public:
    explicit StageTools(QWidget *parent = nullptr);

    void setTotal(int total);
    void setCurrent(int current);
    int total() const { return m_total; }

signals:
    void countChanged(int count);

private:
    void updatePill();
    void applyCount(int value);

    QLineEdit *m_edit = nullptr;
    QLabel *m_pill = nullptr;
    int m_total = 30;
    int m_current = 0;
};
