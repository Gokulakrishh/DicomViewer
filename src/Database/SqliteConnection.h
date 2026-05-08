#pragma once

#include "Utilities/DatabaseSettings.h"
#include "Database/DatabaseConnection.h"

#include <QSqlDatabase>
#include <QString>

/**
 * @brief SQLite implementation of the low-level database connection.
 *
 * Responsibilities:
 * - Create a named Qt SQL connection for the configured SQLite file.
 * - Track connection errors for higher-level services.
 *
 * Assumptions:
 * - SQLite is the local-first persistence backend.
 * - The database stores metadata and annotations, not full DICOM pixel payloads.
 */
class SqliteConnection final : public DatabaseConnection
{
public:
    /**
     * @brief Creates a SQLite connection from application settings.
     * @param databaseSettings Local database file settings.
     */
    explicit SqliteConnection(const DatabaseSettings& databaseSettings);
    ~SqliteConnection() override;

    /**
     * @brief Reports whether a usable SQLite file path is configured.
     * @return True when configuration is sufficient to open the database.
     */
    bool isConfigured() const;

    /**
     * @brief Opens the SQLite database.
     * @return True when the Qt SQL connection is open.
     */
    bool openDB() override;

    /**
     * @brief Closes the SQLite database.
     * @return True when the connection was closed.
     */
    bool closeDB() override;

    /**
     * @brief Executes a raw SQL statement.
     * @param sqlQuery SQL statement to run.
     */
    void execute(const QString& sqlQuery) override;

    /**
     * @brief Checks whether a table exists.
     * @param name Table name.
     * @return True when the table exists.
     */
    bool doesTableExist(const QString& name) override;

    /**
     * @brief Returns the Qt database handle.
     * @return QSqlDatabase associated with this connection.
     */
    QSqlDatabase database() const;

    /**
     * @brief Returns the last connection-level error.
     * @return Human-readable error text.
     */
    QString lastErrorText() const;

private:
    QSqlDatabase m_db;
    QString m_connectionName;
    QString m_lastErrorText;
};
