//
// Created by 11518 on 2024/8/2.
//

#include "zdoublespinbox.h"

ZDoubleSpinBox::ZDoubleSpinBox(QWidget *parent) : QDoubleSpinBox(parent) {
    this->setAlignment(Qt::AlignCenter);
    this->setButtonSymbols(QDoubleSpinBox::ButtonSymbols::NoButtons);
}

ZDoubleSpinBox::~ZDoubleSpinBox() {

}
