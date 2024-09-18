//
// Created by 11518 on 2024/9/16.
//

#ifndef ZTABLE_PRESS_BOT_PROJECT_DATACONTROL_H
#define ZTABLE_PRESS_BOT_PROJECT_DATACONTROL_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

struct DataResult{
    int s_Id;
    QString s_DateTime;
    QString s_JsonData;
    int s_Complete;
    QString s_Line;
};

typedef QVector <DataResult> DataResultList;

class DataControl {
public:
    DataControl();
    ~DataControl();
    bool initSqlDatabaseName();
    bool createResultTableName();
    bool addDataResult(const DataResult &result);
    bool deleteDataResult(int no,QString dateTime);
    bool deleteCurrentDataResultPage(int begin,int size);
    bool selectDataResultList(DataResultList &resultList, int begin, int size);
    bool selectDataResult(DataResult &result, int no,QString dateTime);

    bool deleteAllResult();
    bool selectDataResultCount(int &count);

    void closeSqlDatabase();
private:
    QSqlDatabase m_SqlDatabase;
};


#endif //ZTABLE_PRESS_BOT_PROJECT_DATACONTROL_H
