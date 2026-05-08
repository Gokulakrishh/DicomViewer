#include "Database/SqliteConnection.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

SqliteConnection::SqliteConnection(const DatabaseSettings& databaseSettings)
    : m_connectionName("DicomViewerSqliteConnection_" + QUuid::createUuid().toString(QUuid::Id128))
{
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    m_db.setDatabaseName(databaseSettings.filePath());
}

SqliteConnection::~SqliteConnection()
{
    if (m_db.isOpen())
    {
        m_db.close();
    }

    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool SqliteConnection::isConfigured() const
{
    return !m_db.databaseName().trimmed().isEmpty();
}

bool SqliteConnection::openDB()
{
    if (!m_db.isValid())
    {
        m_lastErrorText = "QSQLITE driver is not available.";
        return false;
    }

    if (!isConfigured())
    {
        m_lastErrorText = "SQLite database path is not configured.";
        return false;
    }

    const QFileInfo databaseFile(m_db.databaseName());
    QDir databaseDirectory(databaseFile.absolutePath());
    if (!databaseDirectory.exists() && !databaseDirectory.mkpath("."))
    {
        m_lastErrorText = QString("Failed to create SQLite database directory: %1").arg(databaseDirectory.absolutePath());
        return false;
    }

    if (m_db.open())
    {
        execute("PRAGMA foreign_keys = ON");
        execute("PRAGMA journal_mode = WAL");
        execute("PRAGMA busy_timeout = 5000");
        m_lastErrorText.clear();
        return true;
    }

    m_lastErrorText = m_db.lastError().text();
    return false;
}

bool SqliteConnection::closeDB()
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

void SqliteConnection::execute(const QString& sqlQuery)
{
    if (!m_db.isOpen())
    {
        return;
    }

    QSqlQuery query(m_db);
    query.exec(sqlQuery);
}

bool SqliteConnection::doesTableExist(const QString& name)
{
    return m_db.isValid() && m_db.tables().contains(name);
}

QSqlDatabase SqliteConnection::database() const
{
    return m_db;
}

QString SqliteConnection::lastErrorText() const
{
    if (!m_lastErrorText.isEmpty())
    {
        return m_lastErrorText;
    }

    return m_db.lastError().text();
}
