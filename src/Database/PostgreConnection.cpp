#include "Database/PostgreConnection.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

PostgreConnection::PostgreConnection(const DatabaseSettings& databaseSettings)
    : m_connectionName("DicomViewerPostgreConnection_" + QUuid::createUuid().toString(QUuid::Id128))
{
    m_db = QSqlDatabase::addDatabase("QPSQL", m_connectionName);
    m_db.setHostName(databaseSettings.hostName());
    m_db.setPort(databaseSettings.port());
    m_db.setDatabaseName(databaseSettings.databaseName());
    m_db.setUserName(databaseSettings.userName());
    m_db.setPassword(databaseSettings.password());
}

PostgreConnection::~PostgreConnection()
{
    if (m_db.isOpen())
    {
        m_db.close();
    }

    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool PostgreConnection::isConfigured() const
{
    return !m_db.databaseName().trimmed().isEmpty() && !m_db.userName().trimmed().isEmpty();
}

bool PostgreConnection::openDB()
{
    if (!m_db.isValid())
    {
        m_lastErrorText = "QPSQL driver is not available.";
        return false;
    }

    if (!isConfigured())
    {
        m_lastErrorText = "PostgreSQL is not configured. Update config.ini with databaseName and userName.";
        return false;
    }

    if (m_db.open())
    {
        m_lastErrorText.clear();
        return true;
    }

    m_lastErrorText = m_db.lastError().text();
    return false;
}

bool PostgreConnection::closeDB()
{
    if (!m_db.isValid())
    {
        return false;
    }

    if (m_db.isOpen())
    {
        m_db.close();
    }

    return true;
}

void PostgreConnection::execute(const QString& sqlQuery)
{
    if (!m_db.isOpen())
    {
        return;
    }

    QSqlQuery query(m_db);
    query.exec(sqlQuery);
}

bool PostgreConnection::doesTableExist(const QString& name)
{
    return m_db.isValid() && m_db.tables().contains(name);
}

QSqlDatabase PostgreConnection::database() const
{
    return m_db;
}

QString PostgreConnection::lastErrorText() const
{
    if (!m_lastErrorText.isEmpty())
    {
        return m_lastErrorText;
    }

    return m_db.lastError().text();
}
