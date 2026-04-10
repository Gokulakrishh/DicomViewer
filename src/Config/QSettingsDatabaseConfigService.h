#pragma once

#include "Config/DatabaseSettings.h"

#include <QString>

class QSettingsDatabaseConfigService
{
public:
    QSettingsDatabaseConfigService();

    DatabaseSettings loadDatabaseSettings() const;
    QString configFilePath() const;

private:
    QString resolveConfigFilePath() const;

private:
    QString m_configFilePath;
};
