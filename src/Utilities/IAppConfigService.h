#pragma once

#include "Utilities/DatabaseSettings.h"
#include "Utilities/AiServiceSettings.h"
#include "Utilities/VolumeValidationSettings.h"

#include <QString>

class IAppConfigService
{
public:
    virtual ~IAppConfigService() = default;

    virtual DatabaseSettings loadDatabaseSettings() const = 0;
    virtual AiServiceSettings loadAiServiceSettings() const = 0;
    virtual VolumeValidationSettings loadVolumeValidationSettings() const = 0;
    virtual QString loadAiApiKey(QString* errorMessage = nullptr) const = 0;
    virtual bool saveAiApiKey(const QString& apiKey, QString* errorMessage = nullptr) = 0;
    virtual QString configFilePath() const = 0;
};
