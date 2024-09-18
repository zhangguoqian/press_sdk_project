//
// Created by 11518 on 2024/8/2.
//

#ifndef ZTABLE_PRESS_BOT_PROJECT_PRESSCHARTVIEW_H
#define ZTABLE_PRESS_BOT_PROJECT_PRESSCHARTVIEW_H

#include <QChartView>
#include <QValueAxis>
#include <QLineSeries>
#include <QSplineSeries>

using namespace QtCharts;

class PressChartView : public  QtCharts::QChartView{
Q_OBJECT
protected:
    void timerEvent(QTimerEvent *event) override;

public:
    explicit PressChartView(QWidget *parent = nullptr);

    ~PressChartView() override;

    void appendPressData(const QPointF &pointF);

    QString lineToJsonString() const;


private:
    QValueAxis *mpXAxis;
    QValueAxis *mpYAxis;

    QLineSeries *mpLineSeries;

};


#endif //ZTABLE_PRESS_BOT_PROJECT_PRESSCHARTVIEW_H
