#include "Database/PostgreConnection.h"

PostgreConnection::PostgreConnection()
{
    QSqlDatabase::addDatabase();
}

PostgreConnection::~PostgreConnection()
{
    if(m_db.isOpen())
        m_db.close();

    QSqlDatabase::removeDatabase();
}

bool PostgreConnection::openDB()
{

}

bool PostgreConnection::closeDB()
{

}

void PostgreConnection::execute(const QString& sqlQuery)
{

}

bool PostgreConnection::doesTableExist(const QString& name)
{

}
