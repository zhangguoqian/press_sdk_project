#pragma once

#include <QTableWidget>
#include <QVector>

enum class StageColumnType { Number, Time, Flag };

struct StageColumn {
    QString title;
    QString unit;
    StageColumnType type = StageColumnType::Number;
    int decimals = 1;
    double minimum = 0.0;
    double maximum = 100.0;
};

class StageDelegate;
class StageHeaderView;

// 阶段数据表：可编辑数值、点击编辑计时、点击切换 Y/— 开关
class StageTable : public QTableWidget
{
    Q_OBJECT
public:
    explicit StageTable(QWidget *parent = nullptr);

    void setColumns(const QVector<StageColumn> &columns, int indexWidth = 64);
    void setStageRowCount(int rows);

    void setNumber(int row, int column, double value);
    void setTime(int row, int column, int seconds);
    void setFlag(int row, int column, bool value);

    double number(int row, int column) const;
    int time(int row, int column) const;
    bool flag(int row, int column) const;

    void setCurrentStage(int stage);
    void setColumnVisible(int column, bool visible);

    const StageColumn *columnSpec(int column) const;
    int m_currentStage = 0;

signals:
    void numberEdited(int row, int column, double value);
    void flagToggled(int row, int column);
    void timeClicked(int row, int column);

private:
    friend class StageDelegate;
    friend class StageHeaderView;

    QVector<StageColumn> m_columns;
    int m_indexWidth = 64;
};
