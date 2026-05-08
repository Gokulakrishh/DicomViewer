#pragma once

#include <QString>

/**
 * @brief Local database configuration.
 *
 * The application currently uses a local SQLite file to avoid embedded external
 * database credentials in distributed builds.
 */
class DatabaseSettings
{
public:
    /** @brief Returns the configured database file path. */
    const QString& filePath() const { return m_filePath; }

    /** @brief Sets the database file path. */
    void setFilePath(const QString& filePath) { m_filePath = filePath; }

    /**
     * @brief Reports whether a database file path is configured.
     * @return True when filePath is non-empty.
     */
    bool isConfigured() const
    {
        return !m_filePath.trimmed().isEmpty();
    }

private:
    QString m_filePath;
};
