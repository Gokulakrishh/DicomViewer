#pragma once

#include "AI/AiChatTypes.h"

class IAiAssistantService
{
public:
    virtual ~IAiAssistantService() = default;

    virtual QString providerId() const = 0;
    virtual bool isConfigured() const = 0;
    virtual QVector<AiModelInfo> availableModels(QString* errorMessage = nullptr) const = 0;
    virtual AiChatResponse sendChat(const AiChatRequest& request) const = 0;
};
