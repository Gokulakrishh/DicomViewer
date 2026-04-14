#pragma once

#include "Utilities/DatabaseSettings.h"
#include "Database/DatabaseConnection.h"

#include <QSqlDatabase>
#include <QString>

class PostgreConnection final : public DatabaseConnection
{
public:
    explicit PostgreConnection(const DatabaseSettings& databaseSettings);
    ~PostgreConnection() override;

    bool isConfigured() const;
    bool openDB() override;
    bool closeDB() override;
    void execute(const QString& sqlQuery) override;
    bool doesTableExist(const QString& name) override;

    QSqlDatabase database() const;
    QString lastErrorText() const;

private:
    QSqlDatabase m_db;
    QString m_connectionName;
    QString m_lastErrorText;
};
