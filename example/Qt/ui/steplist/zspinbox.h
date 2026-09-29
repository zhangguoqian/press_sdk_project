//
// Created by 11518 on 2024/8/2.
//

#ifndef ZTABLE_PRESS_BOT_PROJECT_ZSPINBOX_H
#define ZTABLE_PRESS_BOT_PROJECT_ZSPINBOX_H

#include <QSpinBox>

class ZSpinBox : public QSpinBox{
public:
    explicit ZSpinBox(QWidget *parent = nullptr);

    ~ZSpinBox() override;
};


#endif //ZTABLE_PRESS_BOT_PROJECT_ZSPINBOX_H
