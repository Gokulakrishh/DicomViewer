#pragma once

#include "Utilities/IAppConfigService.h"

#include <QByteArray>
#include <QVariant>
#include <QString>

/**
 * @brief QSettings-backed application configuration service.
 *
 * Responsibilities:
 * - Resolve config file location.
 * - Load database, AI, and validation settings from file/environment.
 *
 * Assumptions:
 * - This service is suitable for local development/baseline builds; secret
 *   storage should move to OS-level facilities for regulated distribution.
 */
class QSettingsAppConfigService final : public IAppConfigService
{
public:
    /** @brief Creates the configuration service. */
    QSettingsAppConfigService();

    /** @brief Loads local database settings. */
    DatabaseSettings loadDatabaseSettings() const override;
    /** @brief Loads optional AI assistant settings. */
    AiServiceSettings loadAiServiceSettings() const override;
    /** @brief Loads volume validation settings. */
    VolumeValidationSettings loadVolumeValidationSettings() const override;
    /** @brief Loads the AI API key. */
    QString loadAiApiKey(QString* errorMessage = nullptr) const override;
    /** @brief Saves the AI API key. */
    bool saveAiApiKey(const QString& apiKey, QString* errorMessage = nullptr) override;
    /** @brief Returns the resolved config file path. */
    QString configFilePath() const override;

private:
    QString defaultDatabaseFilePath() const;
    QVariant readValue(const QString& group, const QString& key, const QVariant& defaultValue = {}) const;
    QString environmentOverrideKey(const QString& group, const QString& key) const;
    QString resolveConfigFilePath() const;

private:
    QString m_configFilePath;
};
