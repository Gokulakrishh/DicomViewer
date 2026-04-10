#pragma once

#include <QString>

class DatabaseSettings
{
public:
    const QString& hostName() const { return m_hostName; }
    const QString& databaseName() const { return m_databaseName; }
    const QString& userName() const { return m_userName; }
    const QString& password() const { return m_password; }
    int port() const { return m_port; }

    void setHostName(const QString& hostName) { m_hostName = hostName; }
    void setDatabaseName(const QString& databaseName) { m_databaseName = databaseName; }
    void setUserName(const QString& userName) { m_userName = userName; }
    void setPassword(const QString& password) { m_password = password; }
    void setPort(int port) { m_port = port; }

    bool isConfigured() const
    {
        return !m_databaseName.trimmed().isEmpty() && !m_userName.trimmed().isEmpty();
    }

private:
    QString m_hostName;
    QString m_databaseName;
    QString m_userName;
    QString m_password;
    int m_port{5432};
};
