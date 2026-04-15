#pragma once

#include "AI/IAiAssistantService.h"

#include "Utilities/AiServiceSettings.h"

#include <memory>

class IAiServerClient;

class GeminiAiAssistantService final : public IAiAssistantService
{
public:
    GeminiAiAssistantService(
        AiServiceSettings settings,
        std::shared_ptr<IAiServerClient> serverClient);

    QString providerId() const override;
    bool isConfigured() const override;
    QVector<AiModelInfo> availableModels(QString* errorMessage = nullptr) const override;
    AiChatResponse sendChat(const AiChatRequest& request) const override;

private:
    QString normalizedModelId(const QString& modelName) const;
    QByteArray buildRequestBody(const AiChatRequest& request) const;
    AiChatResponse parseResponse(const QByteArray& responseBody) const;
    QString modelForRequest(const AiChatRequest& request) const;

private:
    AiServiceSettings m_settings;
    std::shared_ptr<IAiServerClient> m_serverClient;
};
