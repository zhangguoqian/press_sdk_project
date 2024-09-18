//
// Created by 11518 on 2024/8/2.
//

#include "zspinbox.h"

ZSpinBox::ZSpinBox(QWidget *parent) : QSpinBox(parent) {
    this->setAlignment(Qt::AlignCenter);
    this->setButtonSymbols(QDoubleSpinBox::ButtonSymbols::NoButtons);
}

ZSpinBox::~ZSpinBox() {

}
