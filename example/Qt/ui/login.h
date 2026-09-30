//
// Created by 11518 on 2026/9/12.
//

#ifndef PRESS_SDK_LOGIN_H
#define PRESS_SDK_LOGIN_H

#include <QDialog>


QT_BEGIN_NAMESPACE

namespace Ui
{
    class Login;
}

QT_END_NAMESPACE

class Login : public QDialog
{
    Q_OBJECT

public:
    explicit Login(QWidget* parent = nullptr);
    ~Login() override;

    void initPortList();

private:
    Ui::Login* ui;
private Q_SLOTS:
    void slotConnect();
};


#endif //PRESS_SDK_LOGIN_H