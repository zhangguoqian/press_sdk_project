//
// Created by 11518 on 2024/9/16.
//

#include "datacontrol.h"
#include <QDebug>


#define DATABASE_DRIVER "QSQLITE"
#define DATABASE_NAME "data.db"
#define DATABASE_USER "user"
#define DATABASE_PASSWORD "zkls2343"

/*************** Table Result ******************/
#define CREATE_TABLE_RESULT \
"CREATE TABLE IF NOT EXISTS \"Result\" (" \
"\"No\"	INTEGER NOT NULL UNIQUE,"\
"\"DateTime\"	TEXT,"\
"\"JsonData\"	TEXT,"\
"\"Complete\"	INTEGER,"\
"\"Line\"	TEXT,"\
"PRIMARY KEY(\"No\" AUTOINCREMENT)"\
");"

#define NO "No"
#define DATETIME "DateTime"
#define JSONDATA "JsonData"
#define COMPLETE "Complete"
#define LINE "Line"

#define DOT_NO ":No"
#define DOT_DATETIME ":DateTime"
#define DOT_JSONDATA ":JsonData"
#define DOT_COMPLETE ":Complete"
#define DOT_LINE ":Line"

#define ADD_RESULT \
"INSERT into Result(DateTime,JsonData,Complete,Line)"\
"VALUES(:DateTime,:JsonData,:Complete,:Line);"
#define DELETE_ALL_RESULT "DELETE FROM Result;"
#define DELETE_ONE_RESULT "DELETE FROM Result WHERE No = :No AND DateTime = :DateTime;"
#define DELETE_ONE_PAGE_RESULT "delete FROM Result where No in(select No FROM Result order by No DESC limit %0 offset %1);"

#define SELECT_ONE_PAGE_RESULT "SELECT * FROM Result order by No DESC limit %0 offset %1;"
#define SELECT_ONE_RESULT "SELECT * FROM Result where No = '%0' and DateTime = '%1';"

#define SELECT_COUNT_RESULT "SELECT COUNT(*) FROM Result;"

DataControl::DataControl() {
}

bool DataControl::initSqlDatabaseName() {
    QStringList dataDriveList = QSqlDatabase::drivers();
    if(!dataDriveList.contains(DATABASE_DRIVER)){
        return false;
    }
    m_SqlDatabase = QSqlDatabase::addDatabase(DATABASE_DRIVER);;
    m_SqlDatabase.setDatabaseName(DATABASE_NAME);
    m_SqlDatabase.setUserName(DATABASE_USER);
    m_SqlDatabase.setPassword(DATABASE_PASSWORD);
    return m_SqlDatabase.open();
}

DataControl::~DataControl() {

}

void DataControl::closeSqlDatabase() {
    if(m_SqlDatabase.isOpen()){
        m_SqlDatabase.close();
    }
}

bool DataControl::createResultTableName() {
    QSqlQuery sqlQuery;
//    QString cmd = CREATE_TABLE_RESULT;
    return sqlQuery.exec(CREATE_TABLE_RESULT);
}

bool DataControl::addDataResult(const DataResult &result) {
    QSqlQuery sqlQuery;
    sqlQuery.prepare(ADD_RESULT);
    sqlQuery.bindValue(DOT_DATETIME,result.s_DateTime);
    sqlQuery.bindValue(DOT_JSONDATA,result.s_JsonData);
    sqlQuery.bindValue(DOT_COMPLETE,result.s_Complete);
    sqlQuery.bindValue(DOT_LINE,result.s_Line);
    return sqlQuery.exec();
}

bool DataControl::deleteAllResult() {
    QSqlQuery sqlQuery;
    return sqlQuery.exec(DELETE_ALL_RESULT);
}

bool DataControl::deleteCurrentDataResultPage(int page, int size) {
    QSqlQuery sqlQuery;
    bool execInfo = sqlQuery.exec(QString(DELETE_ONE_PAGE_RESULT).arg(size).arg(page));
    return execInfo;
}

bool DataControl::selectDataResultList(DataResultList &resultList, int page, int size) {
    resultList.clear();
    QSqlQuery sqlQuery;
    bool execInfo = sqlQuery.exec(QString(SELECT_ONE_PAGE_RESULT).arg(size).arg(page));
    if(execInfo){
        while (sqlQuery.next()) {
            DataResult data;
            data.s_Id = sqlQuery.value(NO).toInt();
            data.s_DateTime = sqlQuery.value(DATETIME).toString();
            data.s_JsonData = sqlQuery.value(JSONDATA).toString();
            data.s_Complete = sqlQuery.value(COMPLETE).toInt();
            data.s_Line = sqlQuery.value(LINE).toString();
            resultList.append(data);
        }
    }
    return execInfo;
}

bool DataControl::deleteDataResult(int no, QString dateTime) {
    QSqlQuery sqlQuery;
    sqlQuery.prepare(DELETE_ONE_RESULT);
    sqlQuery.bindValue(DOT_NO,no);
    sqlQuery.bindValue(DOT_DATETIME,dateTime);
    return sqlQuery.exec();
}

bool DataControl::selectDataResultCount(int &count) {
    QSqlQuery sqlQuery;
    bool execInfo = sqlQuery.exec(SELECT_COUNT_RESULT);
    while (sqlQuery.next()){
        count = sqlQuery.value(0).toInt();
    }
    return execInfo;
}

bool DataControl::selectDataResult(DataResult &result, int no, QString dateTime) {

    QSqlQuery sqlQuery;
    bool execInfo = sqlQuery.exec(QString(SELECT_ONE_RESULT).arg(no).arg(dateTime));
    if(execInfo){
        while (sqlQuery.next()) {
            result.s_Id = sqlQuery.value(NO).toInt();
            result.s_DateTime = sqlQuery.value(DATETIME).toString();
            result.s_JsonData = sqlQuery.value(JSONDATA).toString();
            result.s_Complete = sqlQuery.value(COMPLETE).toInt();
            result.s_Line = sqlQuery.value(LINE).toString();
        }
    }
    return execInfo;
}
