//
// Created by 11518 on 2024/8/2.
//

#ifndef ZTABLE_PRESS_BOT_PROJECT_STEPTABLEWIDGET_H
#define ZTABLE_PRESS_BOT_PROJECT_STEPTABLEWIDGET_H

#include <QTableWidget>
#include "zdoublespinbox.h"
#include "zspinbox.h"

typedef QVector<ZDoubleSpinBox *> DBoxVector;
typedef QVector<ZSpinBox *> SBoxVector;
typedef QVector<QTableWidgetItem *> TItemVector;

class StepTableWidget : public QTableWidget{
    Q_OBJECT
public:
    explicit StepTableWidget(QWidget *parent = nullptr);
    ~StepTableWidget() override;

    void setStepValueCount(int value);

    int getMaxStepValue() const;

    void setMaxStepValue(int mMaxStepValue);

    void setMpaVisible(bool isVisible);

    void updateData();
    void initDBoxVector(int count);

    void setUpPressValue(const std::vector<float> &list);
    void setDownPressValue(const std::vector<float> &list);
    void setTimePressValue(const std::vector<uint32_t> &list);

    std::vector<float> getUpPressValue();
    std::vector<float> getDownPressValue();
    std::vector<uint32_t> getTimePressValue();

public slots:
    void slotSetValueChanged(int value);

private:
    int m_MaxStepValue;

    TItemVector mpStepItem;
    DBoxVector mpUpPressSpinBoxVector;
    DBoxVector mpDownPressSpinBoxVector;
    SBoxVector mpTimePressSpinBoxVector;
    DBoxVector mpMPaSpinBoxVector;


};


#endif //ZTABLE_PRESS_BOT_PROJECT_STEPTABLEWIDGET_H
