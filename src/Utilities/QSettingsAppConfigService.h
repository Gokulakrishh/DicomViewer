#pragma once

#include "Utilities/IAppConfigService.h"

#include <QByteArray>
#include <QVariant>
#include <QString>

class QSettingsAppConfigService final : public IAppConfigService
{
public:
    QSettingsAppConfigService();

    DatabaseSettings loadDatabaseSettings() const override;
    AiServiceSettings loadAiServiceSettings() const override;
    VolumeValidationSettings loadVolumeValidationSettings() const override;
    QString loadAiApiKey(QString* errorMessage = nullptr) const override;
    bool saveAiApiKey(const QString& apiKey, QString* errorMessage = nullptr) override;
    QString configFilePath() const override;

private:
    QVariant readValue(const QString& group, const QString& key, const QVariant& defaultValue = {}) const;
    QString environmentOverrideKey(const QString& group, const QString& key) const;
    QString resolveConfigFilePath() const;

private:
    QString m_configFilePath;
};
