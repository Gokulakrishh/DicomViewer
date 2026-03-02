#pragma once

#include <QString>
#include <boost/noncopyable.hpp>

class DatabaseConnection : private boost::noncopyable
{
public:

    virtual ~DatabaseConnection() = default;

    virtual bool openDB() = 0;
    virtual bool closeDB() = 0;
    virtual bool doesTableExist(const QString& name) = 0;
    virtual void execute(const QString& sqlQuery) = 0;
    //later implement for cloud options
};

