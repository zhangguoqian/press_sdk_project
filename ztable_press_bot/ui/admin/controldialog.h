//
// Created by 11518 on 2024/8/20.
//

#ifndef ZTABLE_PRESS_BOT_PROJECT_CONTROLDIALOG_H
#define ZTABLE_PRESS_BOT_PROJECT_CONTROLDIALOG_H

#include <QDialog>


QT_BEGIN_NAMESPACE
namespace Ui { class ControlDialog; }
QT_END_NAMESPACE

class ControlDialog : public QDialog {
Q_OBJECT

public:
    explicit ControlDialog(QWidget *parent = nullptr);

    ~ControlDialog() override;

private:
    Ui::ControlDialog *ui;
};


#endif //ZTABLE_PRESS_BOT_PROJECT_CONTROLDIALOG_H
