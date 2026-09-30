//
// Created by 11518 on 2024/8/2.
//

#ifndef PRESS_SDK_PROJECT_ZSPINBOX_H
#define PRESS_SDK_PROJECT_ZSPINBOX_H

#include <QSpinBox>

class ZSpinBox : public QSpinBox{
public:
    explicit ZSpinBox(QWidget *parent = nullptr);

    ~ZSpinBox() override;
};


#endif //PRESS_SDK_PROJECT_ZSPINBOX_H