//
// Created by 11518 on 2024/8/20.
//

// You may need to build the project (run Qt uic code generator) to get "ui_ControlDialog.h" resolved

#include "controldialog.h"
#include "ui_ControlDialog.h"


ControlDialog::ControlDialog(QWidget *parent) :
        QDialog(parent), ui(new Ui::ControlDialog) {
    ui->setupUi(this);
}

ControlDialog::~ControlDialog() {
    delete ui;
}
