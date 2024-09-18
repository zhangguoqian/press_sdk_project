//
// Created by 11518 on 2024/7/29.
//
#include <QApplication>
#include <QStyleFactory>
#include <QDebug>
#include "ui/uihome.h"
#include "ui/uitranslate.h"
#include "pcrhead.h"

ActualPressData *epActualPressData = new ActualPressData();
ReadOnlySetData *epReadOnlySetData = new ReadOnlySetData();
DataControl *epDataControl = new DataControl();
Machine *epMachine = new Machine();

void deleteApp();

int main(int argc,char *argv[]){
    QApplication a(argc,argv);
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    if(!epActualPressData->openJsonFile(APP::ActualPressDataPath)){
        epActualPressData->init(DEFAULT_MAX_STEP);
    }
    epReadOnlySetData->init();

//    if(!epReadOnlySetData->openJsonFile(APP::ReadOnlySetDataPath)){
//        epReadOnlySetData->init();
//    }

    if(!epDataControl->initSqlDatabaseName()){
        qDebug() << "database init error.";
    }
    if(!epDataControl->createResultTableName()){
        qDebug() << "database create result table's name error.";
    }

    UiHome uiHome;

    uiHome.show();

    atexit(deleteApp);

    return QApplication::exec();
}

void deleteApp(){
//    epMachine->quitThread();
//    if(epMachine->isRunning()){
//        epMachine->quit();
//        epMachine->wait();
//    }
//    delete epMachine;
//    epReadOnlySetData->saveJsonFile(APP::ReadOnlySetDataPath);
    epActualPressData->saveJsonFile(APP::ActualPressDataPath);
    epDataControl->closeSqlDatabase();
    delete epDataControl;
    delete epReadOnlySetData;
    delete epActualPressData;
}