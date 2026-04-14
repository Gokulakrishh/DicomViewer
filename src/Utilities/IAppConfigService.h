#pragma once

#include "Utilities/DatabaseSettings.h"
#include "Utilities/VolumeValidationSettings.h"

#include <QString>

class IAppConfigService
{
public:
    virtual ~IAppConfigService() = default;

    virtual DatabaseSettings loadDatabaseSettings() const = 0;
    virtual VolumeValidationSettings loadVolumeValidationSettings() const = 0;
    virtual QString configFilePath() const = 0;
};
