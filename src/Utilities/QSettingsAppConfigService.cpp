#include "Utilities/QSettingsAppConfigService.h"

#include "Utilities/ApplicationPaths.h"

#include <QByteArray>
#include <QProcessEnvironment>
#include <QSettings>

namespace
{
constexpr auto kEncryptedValuePrefix = "enc:";
constexpr auto kApiKeyEncryptionKeyword = "all is well";

QString encryptApiKeyValue(const QString& apiKey)
{
    if (apiKey.isEmpty())
    {
        return QString();
    }

    const QByteArray plainBytes = apiKey.toUtf8();
    const QByteArray keywordBytes(kApiKeyEncryptionKeyword);
    QByteArray encryptedBytes = plainBytes;
    for (int index = 0; index < encryptedBytes.size(); ++index)
    {
        encryptedBytes[index] =
            static_cast<char>(plainBytes.at(index) ^ keywordBytes.at(index % keywordBytes.size()));
    }

    return QString::fromLatin1(kEncryptedValuePrefix) + QString::fromLatin1(encryptedBytes.toBase64());
}

QString decryptApiKeyValue(const QString& storedValue, QString* errorMessage)
{
    if (!storedValue.startsWith(kEncryptedValuePrefix))
    {
        return storedValue.trimmed();
    }

    const int prefixLength = static_cast<int>(std::char_traits<char>::length(kEncryptedValuePrefix));
    const QByteArray encodedBytes = storedValue.mid(prefixLength).toLatin1();
    const QByteArray encryptedBytes = QByteArray::fromBase64(encodedBytes);
    if (encryptedBytes.isEmpty() && !encodedBytes.isEmpty())
    {
        if (errorMessage)
        {
            *errorMessage = "Failed to decode encrypted AI API key from config.ini.";
        }
        return QString();
    }

    const QByteArray keywordBytes(kApiKeyEncryptionKeyword);
    QByteArray plainBytes = encryptedBytes;
    for (int index = 0; index < plainBytes.size(); ++index)
    {
        plainBytes[index] =
            static_cast<char>(encryptedBytes.at(index) ^ keywordBytes.at(index % keywordBytes.size()));
    }

    return QString::fromUtf8(plainBytes).trimmed();
}
}

QSettingsAppConfigService::QSettingsAppConfigService()
    : m_configFilePath(resolveConfigFilePath())
{
}

DatabaseSettings QSettingsAppConfigService::loadDatabaseSettings() const
{
    DatabaseSettings databaseSettings;
    databaseSettings.setFilePath(defaultDatabaseFilePath());
    return databaseSettings;
}

AiServiceSettings QSettingsAppConfigService::loadAiServiceSettings() const
{
    AiServiceSettings aiServiceSettings;
    const QString providerValue = readValue("ai", "provider", "none").toString().trimmed().toLower();
    if (providerValue == "gemini")
    {
        aiServiceSettings.provider = AiProvider::Gemini;
    }
    else if (providerValue == "local")
    {
        aiServiceSettings.provider = AiProvider::LocalServer;
    }
    else
    {
        aiServiceSettings.provider = AiProvider::None;
    }

    aiServiceSettings.baseUrl =
        readValue("ai", "baseUrl", "https://generativelanguage.googleapis.com").toString();
    aiServiceSettings.apiKey = loadAiApiKey();
    aiServiceSettings.model = readValue("ai", "model", "gemini-2.5-flash").toString();

    const QString reasoningValue =
        readValue("ai", "defaultReasoningLevel", "medium").toString().trimmed().toLower();
    if (reasoningValue == "low")
    {
        aiServiceSettings.defaultReasoningLevel = AiReasoningLevel::Low;
    }
    else if (reasoningValue == "high")
    {
        aiServiceSettings.defaultReasoningLevel = AiReasoningLevel::High;
    }
    else
    {
        aiServiceSettings.defaultReasoningLevel = AiReasoningLevel::Medium;
    }

    aiServiceSettings.requestTimeoutMs =
        readValue("ai", "requestTimeoutMs", aiServiceSettings.requestTimeoutMs).toInt();
    aiServiceSettings.maxOutputTokens =
        readValue("ai", "maxOutputTokens", aiServiceSettings.maxOutputTokens).toInt();

    return aiServiceSettings;
}

QString QSettingsAppConfigService::loadAiApiKey(QString* errorMessage) const
{
    const QString envKey = environmentOverrideKey("ai", "apiKey");
    const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    if (environment.contains(envKey))
    {
        return environment.value(envKey);
    }

    return decryptApiKeyValue(readValue("ai", "apiKey").toString().trimmed(), errorMessage);
}

bool QSettingsAppConfigService::saveAiApiKey(const QString& apiKey, QString* errorMessage)
{
    QSettings settings(m_configFilePath, QSettings::IniFormat);
    settings.beginGroup("ai");
    settings.setValue("apiKey", encryptApiKeyValue(apiKey.trimmed()));
    settings.endGroup();
    settings.sync();

    if (settings.status() != QSettings::NoError)
    {
        if (errorMessage)
        {
            *errorMessage = QString("Failed to store API key in %1.").arg(m_configFilePath);
        }
        return false;
    }

    return true;
}

QString QSettingsAppConfigService::configFilePath() const
{
    return m_configFilePath;
}

QString QSettingsAppConfigService::defaultDatabaseFilePath() const
{
    return ApplicationPaths::databaseFilePath();
}

QVariant QSettingsAppConfigService::readValue(const QString& group, const QString& key, const QVariant& defaultValue) const
{
    const QString envKey = environmentOverrideKey(group, key);
    const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    if (environment.contains(envKey))
    {
        return environment.value(envKey);
    }

    QSettings settings(m_configFilePath, QSettings::IniFormat);
    settings.beginGroup(group);
    const QVariant value = settings.value(key, defaultValue);
    settings.endGroup();
    return value;
}

QString QSettingsAppConfigService::environmentOverrideKey(const QString& group, const QString& key) const
{
    return QString("DICOMVIEWER_%1_%2").arg(group.toUpper(), key.toUpper());
}

QString QSettingsAppConfigService::resolveConfigFilePath() const
{
    return ApplicationPaths::configFilePath();
}
