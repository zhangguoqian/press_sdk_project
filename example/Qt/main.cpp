//
// Created by 11518 on 2024/7/29.
//
#include <QApplication>
#include <QDebug>
#include "ui/login.h"
#include "ui/uihome.h"
#include "zgq.h"
#include "ui/admin/controldialog.h"

Machine* epMachine = nullptr;



int main(int argc, char* argv[])
{
    epMachine = new Machine();
    QApplication a(argc, argv);
    Login login;
    if (epMachine->connect("192.168.0.100:8010",TcpSocketPortType))
    {
        ControlDialog dialog;
        dialog.exec();
    }
    // if (login.exec() == QDialog::Accepted)
    // {
    //     UiHome uiHome;
    //     uiHome.show();
    //     epMachine->run();
    //     return QApplication::exec();
    // }
    return 0;
}

