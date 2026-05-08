#pragma once

#include "Utilities/DatabaseSettings.h"
#include "Utilities/AiServiceSettings.h"
#include "Utilities/VolumeValidationSettings.h"

#include <QString>

/**
 * @brief Interface for application configuration access.
 *
 * Responsibilities:
 * - Load local database, AI, and volume validation settings.
 * - Persist optional AI API key configuration.
 *
 * Assumptions:
 * - Secrets storage should be hardened before regulated distribution.
 */
class IAppConfigService
{
public:
    virtual ~IAppConfigService() = default;

    /** @brief Loads local database settings. */
    virtual DatabaseSettings loadDatabaseSettings() const = 0;
    /** @brief Loads optional AI assistant settings. */
    virtual AiServiceSettings loadAiServiceSettings() const = 0;
    /** @brief Loads volume geometry validation settings. */
    virtual VolumeValidationSettings loadVolumeValidationSettings() const = 0;
    /** @brief Loads the AI API key if configured. */
    virtual QString loadAiApiKey(QString* errorMessage = nullptr) const = 0;
    /** @brief Saves the AI API key. */
    virtual bool saveAiApiKey(const QString& apiKey, QString* errorMessage = nullptr) = 0;
    /** @brief Returns the resolved config file path. */
    virtual QString configFilePath() const = 0;
};
