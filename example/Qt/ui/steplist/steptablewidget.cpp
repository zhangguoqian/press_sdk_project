//
// Created by 11518 on 2024/8/2.
//

#include "steptablewidget.h"
#include <QHeaderView>
#include <QDebug>
#include <QStringList>

StepTableWidget::StepTableWidget(QWidget *parent) :
        QTableWidget(parent) ,
        m_MaxStepValue{30}{
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

    // 行数与行标题由代码设置（原先依赖 .ui 中的 <row> 定义）
    // rows and row labels are defined here (previously provided by the .ui file)
    this->setRowCount(5);
    this->setVerticalHeaderLabels(QStringList()
                                  << QStringLiteral("阶段")
                                  << QStringLiteral("设定压力(Ton)")
                                  << QStringLiteral("补压下限(Ton)")
                                  << QStringLiteral("保压时间(ms)")
                                  << QStringLiteral("模具压强(Mpa)"));
    this->verticalHeader()->setDefaultSectionSize(42);
    initDBoxVector(m_MaxStepValue);
}

int StepTableWidget::getMaxStepValue() const {
    return m_MaxStepValue;
}

void StepTableWidget::setMaxStepValue(int maxStepValue) {
    m_MaxStepValue = maxStepValue;
}

void StepTableWidget::initDBoxVector(int count) {
    setColumnCount(m_MaxStepValue);
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

        mpDownPressSpinBoxVector[i] = new ZDoubleSpinBox();
        auto r2 = mpDownPressSpinBoxVector[i];
        this->setCellWidget(2,i,r2);

        mpTimePressSpinBoxVector[i] = new ZSpinBox();
        auto r3 = mpTimePressSpinBoxVector[i];
        this->setCellWidget(3,i,r3);
        r3->setMaximum(99999999);

        mpMPaSpinBoxVector[i] = new ZDoubleSpinBox();
        auto r4 = mpMPaSpinBoxVector[i];
        this->setCellWidget(4,i, r4);
        r4->setAlignment(Qt::AlignCenter);


        // 设定压力列 → 上报主界面写回设备 / report to main window for write-back
        connect(r1, QOverload<double>::of(&ZDoubleSpinBox::valueChanged),this, [this, i](double value){
            emit signalCellValueChanged(1, i, value);
        });

        // 保压值列 / after-pressure column
        connect(r2, QOverload<double>::of(&ZDoubleSpinBox::valueChanged),this, [this, i](double value){
            emit signalCellValueChanged(2, i, value);
        });

        // 保压时间列 / keep-pressure time column
        connect(r3, QOverload<int>::of(&ZSpinBox::valueChanged),this, [this, i](int value){
            emit signalCellValueChanged(3, i, static_cast<double>(value));
        });

        // MPa 为换算显示列(由吨位/缸径推导), 不写回设备
        // MPa column is derived display only, no device write-back
        connect(r4, QOverload<double>::of(&ZDoubleSpinBox::valueChanged),this, [=](double value){
            Q_UNUSED(value);
        });
    }
}

void StepTableWidget::setUpPressValue(const std::vector<float>& list)
{
    for (int i = 0; i < list.size(); ++i)
    {
        mpUpPressSpinBoxVector[i]->setValue(list[i]);
    }
}

void StepTableWidget::setDownPressValue(const std::vector<float>& list)
{
    for (int i = 0; i < list.size(); ++i)
    {
        mpDownPressSpinBoxVector[i]->setValue(list[i]);
    }
}

void StepTableWidget::setTimePressValue(const std::vector<uint32_t>& list)
{
    for (int i = 0; i < list.size(); ++i)
    {
        mpTimePressSpinBoxVector[i]->setValue(list[i]);
    }
}

std::vector<float> StepTableWidget::getUpPressValue()
{
    std::vector<float> list;
    for (int i = 0; i < m_MaxStepValue; ++i) {
        list.push_back(mpUpPressSpinBoxVector[i]->value());
    }
    return list;
}

std::vector<float> StepTableWidget::getDownPressValue()
{
    std::vector<float> list;
    for (int i = 0; i < m_MaxStepValue; ++i) {
        list.push_back(mpDownPressSpinBoxVector[i]->value());
    }
    return list;
}

std::vector<uint32_t> StepTableWidget::getTimePressValue()
{
    std::vector<uint32_t> list;
    for (int i = 0; i < m_MaxStepValue; ++i) {
        list.push_back(mpTimePressSpinBoxVector[i]->value());
    }
    return list;
}


void StepTableWidget::setStepValueCount(int value) {
    for (int i = 0; i < m_MaxStepValue; ++i) {
        this->setColumnHidden(i, false);
        this->setColumnHidden(i,i>=value);
    }
}

void StepTableWidget::slotSetValueChanged(int value) {
    setStepValueCount(value);

}

void StepTableWidget::setMpaVisible(bool isVisible) {
    this->setRowHidden(4,true);
}

void StepTableWidget::updateData() {
    for (auto &var:mpUpPressSpinBoxVector) {
        emit var->valueChanged(var->value());
    }
}

StepTableWidget::~StepTableWidget() = default;