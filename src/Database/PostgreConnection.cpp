#include "Database/PostgreConnection.h"

#include <QSqlQuery>

PostgreConnection::PostgreConnection()
    : m_connectionName("DicomViewerPostgreConnection")
{
    m_db = QSqlDatabase::addDatabase("QPSQL", m_connectionName);
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

bool PostgreConnection::openDB()
{
    return m_db.isValid() && m_db.open();
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
