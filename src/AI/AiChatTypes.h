#pragma once

#include "Utilities/AiServiceSettings.h"

#include <QByteArray>
#include <QString>
#include <QVector>

/**
 * @brief Role assigned to a message in an AI conversation.
 *
 * The role is intentionally provider-neutral so UI and audit code do not depend
 * on one external AI API schema.
 */
enum class AiChatRole
{
    System = 0,
    User,
    Assistant
};

/**
 * @brief One message sent to or received from an AI assistant service.
 *
 * Responsibilities:
 * - Preserve the conversational role and text content.
 * - Carry optional image attachments when the configured provider supports vision.
 *
 * Assumptions:
 * - Attached DICOM renderings are derived viewer snapshots, not original DICOM files.
 * - Callers are responsible for avoiding protected health information disclosure
 *   when AI features are enabled.
 */
struct AiChatMessage
{
    AiChatRole role{AiChatRole::User};
    QString content;

    /**
     * @brief Binary image payload attached to an AI chat message.
     *
     * The payload uses an explicit MIME type so transport implementations can
     * encode it according to the selected provider contract.
     */
    struct ImageAttachment
    {
        QString mimeType;
        QByteArray data;
    };
    QVector<ImageAttachment> imageAttachments;
};

/**
 * @brief Provider model metadata shown to the user.
 *
 * The structure is lightweight and can be refreshed from the provider without
 * changing the rest of the viewer.
 */
struct AiModelInfo
{
    QString id;
    QString displayName;
    bool supportsVision{false};
};

/**
 * @brief Generation options applied to an AI chat request.
 *
 * These settings are separate from the message list so UI defaults, request
 * overrides, and future audit records can be handled independently.
 */
struct AiGenerationOptions
{
    QString modelOverride;
    AiReasoningLevel reasoningLevel{AiReasoningLevel::Medium};
    int maxOutputTokens{2048};
};

/**
 * @brief Complete AI chat request sent by the application.
 *
 * The request is deliberately provider-neutral; concrete services translate it
 * to provider-specific JSON.
 */
struct AiChatRequest
{
    QString conversationId;
    QString conversationTitle;
    QVector<AiChatMessage> messages;
    AiGenerationOptions generationOptions;
};

/**
 * @brief Result returned by an AI assistant service.
 *
 * Failed responses carry an error message instead of throwing, allowing the UI
 * to report recoverable provider or network failures.
 */
struct AiChatResponse
{
    bool success{false};
    QString providerName;
    QString modelName;
    QString answer;
    QString finishReason;
    QString errorMessage;
};
