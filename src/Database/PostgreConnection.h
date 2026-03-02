#pragma once

#include "Database/DatabaseConnection.h"
#include <QSqlDatabase>
#include <QString>

class PostgreConnection : public DatabaseConnection
{
public:
    PostgreConnection();
    ~PostgreConnection();
    bool openDB() override;
    bool closeDB() override;
    void execute(const QString& sqlQuery) override;
    bool doesTableExist(const QString& name) override;
private:
    QSqlDatabase m_db; //since we are using internal database, dont need to set/get hostname, passwor and so
    QString m_connectionName;
};
