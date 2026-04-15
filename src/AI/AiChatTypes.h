#pragma once

#include "Utilities/AiServiceSettings.h"

#include <QByteArray>
#include <QString>
#include <QVector>

enum class AiChatRole
{
    System = 0,
    User,
    Assistant
};

struct AiChatMessage
{
    AiChatRole role{AiChatRole::User};
    QString content;
    struct ImageAttachment
    {
        QString mimeType;
        QByteArray data;
    };
    QVector<ImageAttachment> imageAttachments;
};

struct AiModelInfo
{
    QString id;
    QString displayName;
    bool supportsVision{false};
};

struct AiGenerationOptions
{
    QString modelOverride;
    AiReasoningLevel reasoningLevel{AiReasoningLevel::Medium};
    int maxOutputTokens{2048};
};

struct AiChatRequest
{
    QString conversationId;
    QString conversationTitle;
    QVector<AiChatMessage> messages;
    AiGenerationOptions generationOptions;
};

struct AiChatResponse
{
    bool success{false};
    QString providerName;
    QString modelName;
    QString answer;
    QString finishReason;
    QString errorMessage;
};
