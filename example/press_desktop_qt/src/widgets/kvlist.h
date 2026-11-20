#pragma once

#include <QColor>
#include <QHash>
#include <QWidget>

class QLabel;

// 键值行列表：左标签（含单位）+ 右数值，可带运行状态圆点
class KvList : public QWidget
{
    Q_OBJECT
public:
    explicit KvList(QWidget *parent = nullptr);

    void setRowHeight(int height);

    void addRow(const QString &key, const QString &label,
                const QString &unit = QString(), const QColor &dot = QColor());
    void setValue(const QString &key, const QString &text);
    void setValueColor(const QString &key, const QColor &color);
    QString value(const QString &key) const;

private:
    QHash<QString, QLabel *> m_values;
    QHash<QString, QWidget *> m_rows;
    int m_rowCount = 0;
    int m_rowHeight = 42;
};
