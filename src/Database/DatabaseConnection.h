#pragma once

#include <QString>

class DatabaseConnection
{
public:
    DatabaseConnection() = default;
    DatabaseConnection(const DatabaseConnection&) = delete;
    DatabaseConnection& operator=(const DatabaseConnection&) = delete;
    DatabaseConnection(DatabaseConnection&&) = delete;
    DatabaseConnection& operator=(DatabaseConnection&&) = delete;
    virtual ~DatabaseConnection() = default;

    virtual bool openDB() = 0;
    virtual bool closeDB() = 0;
    virtual bool doesTableExist(const QString& name) = 0;
    virtual void execute(const QString& sqlQuery) = 0;
};
