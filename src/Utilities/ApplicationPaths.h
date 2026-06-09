#pragma once

#include <QString>

/**
 * @brief Central writable-path provider for application-owned local files.
 *
 * Responsibilities:
 * - Create and return the product folder under the user's Documents directory.
 * - Provide stable subpaths for database, audit, diagnostics, and config files.
 * - Keep storage-location decisions out of feature-specific services.
 *
 * Assumptions:
 * - The application is local-first and stores only application metadata,
 *   annotations, logs, and configuration in this folder.
 * - Source DICOM files remain at their original locations and are not copied
 *   into this application folder.
 */
class ApplicationPaths final
{
public:
    /**
     * @brief Returns the product root folder under Documents.
     * @return Absolute path to the application root folder.
     */
    [[nodiscard]] static QString applicationRootDirectory();

    /**
     * @brief Returns the directory used for SQLite and other local data files.
     * @return Absolute path to the data directory.
     */
    [[nodiscard]] static QString dataDirectory();

    /**
     * @brief Returns the local SQLite database file path.
     * @return Absolute path to the SQLite database.
     */
    [[nodiscard]] static QString databaseFilePath();

    /**
     * @brief Returns the directory used for append-only audit files.
     * @return Absolute path to the audit directory.
     */
    [[nodiscard]] static QString auditDirectory();

    /**
     * @brief Returns the JSONL audit file path.
     * @return Absolute path to the audit JSONL file.
     */
    [[nodiscard]] static QString auditFilePath();

    /**
     * @brief Returns the directory used for logs and crash diagnostics.
     * @return Absolute path to the diagnostics directory.
     */
    [[nodiscard]] static QString diagnosticsDirectory();

    /**
     * @brief Returns the application diagnostic log file path.
     * @return Absolute path to the application log.
     */
    [[nodiscard]] static QString applicationLogFilePath();

    /**
     * @brief Returns the crash-report directory.
     * @return Absolute path to the crash-report directory.
     */
    [[nodiscard]] static QString crashReportDirectory();

    /**
     * @brief Returns the directory used for local configuration.
     * @return Absolute path to the config directory.
     */
    [[nodiscard]] static QString configDirectory();

    /**
     * @brief Returns the INI configuration file path.
     * @return Absolute path to the config file.
     */
    [[nodiscard]] static QString configFilePath();

private:
    ApplicationPaths() = delete;
};
