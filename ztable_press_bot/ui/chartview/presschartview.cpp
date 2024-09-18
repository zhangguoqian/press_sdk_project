//
// Created by 11518 on 2024/8/2.
//

#include "presschartview.h"
#include <QObject>
#include <QRandomGenerator64>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

const QString XAxisTitleName = qtTrId("时间/s");
const QString YAxisTitleName = qtTrId("压力/Ton");

PressChartView::PressChartView(QWidget *parent) :
    QtCharts::QChartView(parent),
    mpXAxis(new QValueAxis()),
    mpYAxis(new QValueAxis()),
    mpLineSeries(new QLineSeries()){

    setRenderHint(QPainter::Antialiasing);

    this->chart()->addSeries(mpLineSeries);
    this->chart()->legend()->hide();

    mpXAxis->setTitleText(XAxisTitleName);
    mpYAxis->setTitleText(YAxisTitleName);


    QFont font;
    mpXAxis->setTitleFont(font);
    mpYAxis->setTitleFont(font);

    mpXAxis->setLabelFormat("%d");
    mpYAxis->setLabelFormat("%0.2f");

    mpXAxis->setRange(0,60);
    mpYAxis->setRange(0,25);

//    mpXAxis->setTickType(QValueAxis::TickType::TicksDynamic);
//    mpXAxis->setTickInterval(60);
    mpXAxis->setTickCount(10);
    mpYAxis->setTickCount(6);


    this->chart()->addAxis(mpXAxis,Qt::AlignBottom);
    this->chart()->addAxis(mpYAxis,Qt::AlignLeft);


    mpLineSeries->attachAxis(mpXAxis);
    mpLineSeries->attachAxis(mpYAxis);

//    startTimer(2000);
}

PressChartView::~PressChartView() {
    delete mpLineSeries;
    delete mpYAxis;
    delete mpXAxis;
}

void PressChartView::timerEvent(QTimerEvent *event) {
    static int x = 0;
    int y = QRandomGenerator64::global()->bounded(500);

    mpLineSeries->append(x,y);
    x ++ ;
    this->mpXAxis->setMax(x>mpXAxis->max()?x:mpXAxis->max());
    QObject::timerEvent(event);
}

void PressChartView::appendPressData(const QPointF &pointF) {
    mpLineSeries->append(pointF);
    if(pointF.x() > mpXAxis->max()){
        this->mpXAxis->setMax(pointF.x()+10);
    }
//    this->mpXAxis->setMax(x>mpXAxis->max()?x:mpXAxis->max());
}

QString PressChartView::lineToJsonString() const {
    QJsonArray jsonArrayX;
    QJsonArray jsonArrayY;
    for (auto &var:this->mpLineSeries->pointsVector()) {
        jsonArrayX.append(var.x());
        jsonArrayY.append(var.y());
    }
//    QJsonValue jsonValueX = jsonArrayX;
//    QJsonValue jsonValueY = jsonArrayY;
    QJsonObject jsonObject;
    jsonObject["X"] = jsonArrayX;
    jsonObject["Y"] = jsonArrayY;
    QJsonDocument jsonDocument;
    jsonDocument.setObject(jsonObject);
    return jsonDocument.toJson(QJsonDocument::JsonFormat::Compact);
}

void PressChartView::clearPressData() {
    mpLineSeries->clear();
}

void PressChartView::jsonStringToLine(const QString& jsonData) {
    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData.toLatin1());
    QJsonObject jsonObject = jsonDocument.object();
    QJsonArray jsonArrayX = jsonObject["X"].toArray();
    QJsonArray jsonArrayY = jsonObject["Y"].toArray();
    int sizeX = jsonArrayX.size();
    int sizeY = jsonArrayY.size();
    if(sizeX==sizeY){
        for (int i = 0; i < sizeX; ++i) {
            mpLineSeries->append(jsonArrayX[i].toDouble(),jsonArrayY[i].toDouble());
        }
    }
}

