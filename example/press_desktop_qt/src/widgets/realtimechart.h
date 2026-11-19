#pragma once

#include <QVector>
#include <QWidget>

// 实时曲线：压力（左轴 Ton）+ 上下板温度（右轴 °C），3 线双轴
class RealtimeChart : public QWidget
{
    Q_OBJECT
public:
    explicit RealtimeChart(QWidget *parent = nullptr);

    void addSample(double pressure, double upTemp, double downTemp);
    void clear();
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    double temperatureAxisMax() const;

    static constexpr int kWindow = 360; // 2 Hz × 180 s
    QVector<double> m_pressure;
    QVector<double> m_up;
    QVector<double> m_down;
};
