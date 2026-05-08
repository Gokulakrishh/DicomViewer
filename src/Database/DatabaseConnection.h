#pragma once

#include <QString>

/**
 * @brief Low-level database connection abstraction.
 *
 * Responsibilities:
 * - Own the database driver connection lifecycle.
 * - Expose minimal table/SQL operations needed by concrete services.
 *
 * Assumptions:
 * - Higher-level data mapping belongs in DatabaseService implementations.
 * - Connections are non-copyable because Qt database handles are connection-name
 *   scoped resources.
 */
class DatabaseConnection
{
public:
    DatabaseConnection() = default;
    DatabaseConnection(const DatabaseConnection&) = delete;
    DatabaseConnection& operator=(const DatabaseConnection&) = delete;
    DatabaseConnection(DatabaseConnection&&) = delete;
    DatabaseConnection& operator=(DatabaseConnection&&) = delete;
    virtual ~DatabaseConnection() = default;

    /**
     * @brief Opens the underlying database connection.
     * @return True when the connection is ready for SQL operations.
     */
    virtual bool openDB() = 0;

    /**
     * @brief Closes the underlying database connection.
     * @return True when the close operation completed.
     */
    virtual bool closeDB() = 0;

    /**
     * @brief Checks whether a table exists.
     * @param name Table name to look up.
     * @return True when the table exists in the active database.
     */
    virtual bool doesTableExist(const QString& name) = 0;

    /**
     * @brief Executes a raw SQL statement.
     * @param sqlQuery SQL statement to run.
     */
    virtual void execute(const QString& sqlQuery) = 0;
};
