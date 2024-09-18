#ifndef ZFILE_H
#define ZFILE_H

#include <string>

#ifdef QT_CORE_LIB

#include <QFile>

namespace ZGLOBAL {

#ifdef QT_CORE_LIB

    bool readFile(const QString &filePath, QByteArray &byteArray);

    QByteArray readFile(const QString &fileName);

    bool writeFile(const QString &filePath, const QByteArray &byteArray);

    bool appendFile(const QString &filePath, const QByteArray &byteArray);

    QString getFileMd5(const QString &filePath);
#endif

    bool readFile(const std::string &fileName, std::string &string);

    std::string readFile(const std::string &fileName);

    bool writeFile(const std::string &fileName, const std::string &string);

    bool appendFile(const std::string &fileName, const std::string &string);
};

#endif

#endif // ZFILE_H
