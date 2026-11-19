#pragma once

#include <QFrame>

class QLabel;
class QLineEdit;

// 带单位后缀的数值输入框
class UnitInput : public QFrame
{
    Q_OBJECT
public:
    explicit UnitInput(const QString &suffix, QWidget *parent = nullptr);

    void setRange(double minimum, double maximum, int decimals = 1);
    void setValue(double value);
    double value() const;

signals:
    void valueChanged(double value);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void applyFocusState(bool focused);
    void commit();

    QLineEdit *m_edit = nullptr;
    QLabel *m_suffix = nullptr;
    int m_decimals = 1;
    double m_minimum = 0.0;
    double m_maximum = 10000.0;
};
