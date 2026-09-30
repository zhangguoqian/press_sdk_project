//
// Created by 11518 on 2024/8/2.
//

#ifndef PRESS_SDK_PROJECT_ZDOUBLESPINBOX_H
#define PRESS_SDK_PROJECT_ZDOUBLESPINBOX_H

#include <QDoubleSpinBox>


class ZDoubleSpinBox : public QDoubleSpinBox {
public:
    explicit ZDoubleSpinBox(QWidget *parent = nullptr);

    ~ZDoubleSpinBox() override;
};


#endif //PRESS_SDK_PROJECT_ZDOUBLESPINBOX_H