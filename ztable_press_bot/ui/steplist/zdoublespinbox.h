//
// Created by 11518 on 2024/8/2.
//

#ifndef ZTABLE_PRESS_BOT_PROJECT_ZDOUBLESPINBOX_H
#define ZTABLE_PRESS_BOT_PROJECT_ZDOUBLESPINBOX_H

#include <QDoubleSpinBox>


class ZDoubleSpinBox : public QDoubleSpinBox {
public:
    explicit ZDoubleSpinBox(QWidget *parent = nullptr);

    ~ZDoubleSpinBox() override;
};


#endif //ZTABLE_PRESS_BOT_PROJECT_ZDOUBLESPINBOX_H
