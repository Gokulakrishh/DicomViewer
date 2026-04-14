#pragma once

#include "Utilities/IAppConfigService.h"

#include <QString>

class QSettingsAppConfigService final : public IAppConfigService
{
public:
    QSettingsAppConfigService();

    DatabaseSettings loadDatabaseSettings() const override;
    VolumeValidationSettings loadVolumeValidationSettings() const override;
    QString configFilePath() const override;

private:
    QString resolveConfigFilePath() const;

private:
    QString m_configFilePath;
};
