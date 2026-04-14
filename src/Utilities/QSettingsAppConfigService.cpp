#include "Utilities/QSettingsAppConfigService.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>

QSettingsAppConfigService::QSettingsAppConfigService()
    : m_configFilePath(resolveConfigFilePath())
{
}

DatabaseSettings QSettingsAppConfigService::loadDatabaseSettings() const
{
    DatabaseSettings databaseSettings;
    QSettings settings(m_configFilePath, QSettings::IniFormat);

    settings.beginGroup("database");
    databaseSettings.setHostName(settings.value("hostName", "127.0.0.1").toString());
    databaseSettings.setPort(settings.value("port", 5432).toInt());
    databaseSettings.setDatabaseName(settings.value("databaseName").toString());
    databaseSettings.setUserName(settings.value("userName").toString());
    databaseSettings.setPassword(settings.value("password").toString());
    settings.endGroup();

    return databaseSettings;
}

VolumeValidationSettings QSettingsAppConfigService::loadVolumeValidationSettings() const
{
    VolumeValidationSettings validationSettings;
    QSettings settings(m_configFilePath, QSettings::IniFormat);

    settings.beginGroup("volumeValidation");
    validationSettings.orientationAlignmentTolerance =
        settings.value("orientationAlignmentTolerance", validationSettings.orientationAlignmentTolerance).toDouble();
    validationSettings.spacingTolerance =
        settings.value("spacingTolerance", validationSettings.spacingTolerance).toDouble();
    validationSettings.validateUniformSliceSpacing =
        settings.value("validateUniformSliceSpacing", validationSettings.validateUniformSliceSpacing).toBool();
    settings.endGroup();

    return validationSettings;
}

QString QSettingsAppConfigService::configFilePath() const
{
    return m_configFilePath;
}

QString QSettingsAppConfigService::resolveConfigFilePath() const
{
    const QString applicationDirPath = QCoreApplication::applicationDirPath();
    const QString localConfigPath = QDir(applicationDirPath).filePath("config.ini");
    if (QFileInfo::exists(localConfigPath))
    {
        return localConfigPath;
    }

    const QString parentConfigPath = QDir(applicationDirPath).filePath("../config.ini");
    return QDir::cleanPath(parentConfigPath);
}
