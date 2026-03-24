#include "Database/PostgreConnection.h"

#include <QSqlQuery>
#include <QUuid>

PostgreConnection::PostgreConnection()
    : m_connectionName(QString("DicomViewerPostgreConnection_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
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
    if (!m_db.isValid() || !m_db.isOpen())
    {
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT EXISTS (
            SELECT 1
            FROM information_schema.tables
            WHERE table_schema = current_schema()
              AND table_name = :table_name
        )
    )");
    query.bindValue(":table_name", name);

    if (!query.exec() || !query.next())
    {
        return false;
    }

    return query.value(0).toBool();
}
