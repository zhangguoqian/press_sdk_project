//
// Created by 11518 on 2024/8/2.
//

#ifndef PRESS_SDK_PROJECT_PRESSCHARTVIEW_H
#define PRESS_SDK_PROJECT_PRESSCHARTVIEW_H

#include <QChartView>
#include <QValueAxis>
#include <QLineSeries>
#include <QSplineSeries>

#if QT_VERSION_MAJOR <= 5
using namespace QtCharts;
class PressChartView : public  QtCharts::QChartView{

#else
class PressChartView : public  QChartView{
#endif
Q_OBJECT
protected:
    void timerEvent(QTimerEvent *event) override;

public:
    explicit PressChartView(QWidget *parent = nullptr);

    ~PressChartView() override;

    void appendPressData(const QPointF &pointF);

    QString lineToJsonString() const;

    void jsonStringToLine(const QString& jsonData);

    void clearPressData();

    void setRange(double min, double max);
private:
    QValueAxis *mpXAxis;
    QValueAxis *mpYAxis;

    QLineSeries *mpLineSeries;

};


#endif //PRESS_SDK_PROJECT_PRESSCHARTVIEW_H