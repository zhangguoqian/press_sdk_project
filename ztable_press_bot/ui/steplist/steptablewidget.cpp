//
// Created by 11518 on 2024/8/2.
//

#include "steptablewidget.h"
#include <QHeaderView>
#include <QDebug>
#include "pcrhead.h"
#include "pcrhead.h"

//inline double tonToTypeMpa(double ton,int type, double dmm, double amm, double bmm){
//    double S = 0.0;
//    if(type == 0){
//        return 0.0;
//    }else if(type == 1){
//        S = PI * pow(dmm/2.0 * 0.001,2);
//    }else if(type == 2){
//        S = amm * bmm * 0.000001;
//    }
//
//    return ton * 1000.0 * g / S;
//}

StepTableWidget::StepTableWidget(QWidget *parent) :
        QTableWidget(parent) ,
        m_MaxStepValue{}{
    this->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);

    this->horizontalHeader()->setVisible(false);//不显示水平表头
    this->verticalHeader()->setVisible(true); //显示垂直表头

    this->setFocusPolicy(Qt::FocusPolicy::NoFocus); //设置无焦点
    this->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection); //设置无选中

    this->verticalHeader()->setStretchLastSection(true); //最后一行扩展
    this->horizontalHeader()->setStretchLastSection(true); //最后一行扩展

    this->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Stretch); //自适应尺寸
    this->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Stretch); //自适应尺寸

    this->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);

}

int StepTableWidget::getMaxStepValue() const {
    return m_MaxStepValue;
}

void StepTableWidget::setMaxStepValue(int maxStepValue) {
    m_MaxStepValue = maxStepValue;
    initDBoxVector(m_MaxStepValue);
}

void StepTableWidget::initDBoxVector(int count) {
    setColumnCount(m_MaxStepValue);
    deleteDBoxVector();
    mpStepItem.resize(count);
    mpUpPressSpinBoxVector.resize(count);
    mpDownPressSpinBoxVector.resize(count);
    mpTimePressSpinBoxVector.resize(count);
    mpMPaSpinBoxVector.resize(count);
    for (int i = 0; i < count; ++i) {
        mpStepItem[i] = new QTableWidgetItem(QString::number(i+1));
        auto r0 = mpStepItem[i];
        this->setItem(0,i,r0);
        r0->setTextAlignment(Qt::AlignCenter);
        r0->setFlags(Qt::ItemFlag::ItemIsSelectable);


        mpUpPressSpinBoxVector[i] = new ZDoubleSpinBox();
        auto r1 = mpUpPressSpinBoxVector[i];
        this->setCellWidget(1,i, r1);
        r1->setValue(epActualPressData->s_Ups[i]);
        r1->setRange(epReadOnlySetData->s_MinLimit,epReadOnlySetData->s_MaxLimit);
        r1->setDecimals(epReadOnlySetData->s_PressPrecision);

        mpDownPressSpinBoxVector[i] = new ZDoubleSpinBox();
        auto r2 = mpDownPressSpinBoxVector[i];
        this->setCellWidget(2,i,r2);
        r2->setValue(epActualPressData->s_Downs[i]);
        r2->setDecimals(epReadOnlySetData->s_PressPrecision);

        mpTimePressSpinBoxVector[i] = new ZSpinBox();
        auto r3 = mpTimePressSpinBoxVector[i];
        this->setCellWidget(3,i,r3);
        r3->setValue(epActualPressData->s_Times[i]);
        r3->setRange(0,3600 * 24);
//        r3->setDecimals(epReadOnlySetData->s_PressPrecision);

        mpMPaSpinBoxVector[i] = new ZDoubleSpinBox();
        auto r4 = mpMPaSpinBoxVector[i];
        this->setCellWidget(4,i, r4);
        r4->setAlignment(Qt::AlignCenter);
        r4->setMaximum(99999999999);
        r4->setDecimals(epReadOnlySetData->s_PressPrecision);
//        r4->setReadOnly(true);

        connect(r1, QOverload<double>::of(&ZDoubleSpinBox::valueChanged),this, [=](double value){
            epActualPressData->s_Ups[i] = value;
            r2->setMaximum(r1->value() - epReadOnlySetData->s_Max_Min);
            double mpaValue = APP::tonToTypeMpa(value,epActualPressData->s_Type,epActualPressData->s_D,epActualPressData->s_A,epActualPressData->s_B);
            r4->setValue(mpaValue);
        });

            connect(r2, QOverload<double>::of(&ZDoubleSpinBox::valueChanged),this, [=](double value){
            epActualPressData->s_Downs[i] = value;
        });
        connect(r3, QOverload<int>::of(&ZSpinBox::valueChanged),this, [=](int value){
            epActualPressData->s_Times[i] = value;
        });

        connect(r4, QOverload<double>::of(&ZDoubleSpinBox::valueChanged),this, [=](double value){
            if(epActualPressData->s_Type==0){
                return;
            }
            double tonValue = APP::typeMpaTonTon(value,epActualPressData->s_Type,epActualPressData->s_D,epActualPressData->s_A,epActualPressData->s_B);
            epActualPressData->s_Ups[i] = tonValue;
            r1->setValue(tonValue);
            r2->setMaximum(r1->value() - epReadOnlySetData->s_Max_Min);
        });
    }
}

void StepTableWidget::deleteDBoxVector() {
    for (auto &var:mpUpPressSpinBoxVector) {
        delete var;
    }
}

void StepTableWidget::setStepValueCount(int value) {
    epActualPressData->s_Step = value;
    for (int i = 0; i < m_MaxStepValue; ++i) {
        this->setColumnHidden(i, false);
        this->setColumnHidden(i,i>=value);
    }
}

void StepTableWidget::slotSetValueChanged(int value) {
    setStepValueCount(value);
}

void StepTableWidget::setMpaVisible(bool isVisible) {
    this->setRowHidden(4,!isVisible);
}

void StepTableWidget::updateData() {
////    m_Type = type;
////    m_Diameter = d;
////    m_A = a;
////    m_B = b;
//    epActualPressData->setSType(type);
//    epActualPressData->setSD(d);
//    epActualPressData->setSA(a);
//    epActualPressData->setSB(b);
    for (auto &var:mpUpPressSpinBoxVector) {
        emit var->valueChanged(var->value());
    }
}

//void StepTableWidget::setActualData(const  values) {
//    uint size = values.size();
//    for(int i(0);i<size;i++){
//        mpUpPressSpinBoxVector[i]->setValue(values[i]);
//        mpDownPressSpinBoxVector[i]->setValue(values[i]);
//        mpTimePressSpinBoxVector[i]->setValue(values[i]);
//    }
//
//}


StepTableWidget::~StepTableWidget() = default;