#pragma once

#include "AI/IAiAssistantService.h"

#include "Utilities/AiServiceSettings.h"

#include <memory>

class IAiServerClient;

/**
 * @brief Google Gemini implementation of the optional AI assistant interface.
 *
 * Responsibilities:
 * - Translate provider-neutral chat requests into Gemini JSON payloads.
 * - Parse Gemini responses into application-level chat results.
 * - Keep model normalization and transport details outside the main viewer.
 *
 * Assumptions:
 * - The service is optional and may be disabled in clinical/professional builds.
 * - The caller is responsible for PHI governance before sending any image or
 *   text content to an external provider.
 */
class GeminiAiAssistantService final : public IAiAssistantService
{
public:
    /**
     * @brief Creates a Gemini assistant service.
     * @param settings Provider settings including API key and default model.
     * @param serverClient HTTP client used for provider requests.
     */
    GeminiAiAssistantService(
        AiServiceSettings settings,
        std::shared_ptr<IAiServerClient> serverClient);

    /**
     * @brief Returns the Gemini provider id.
     * @return Stable provider identifier.
     */
    QString providerId() const override;

    /**
     * @brief Reports whether Gemini credentials are configured.
     * @return True when the service has enough settings to send requests.
     */
    bool isConfigured() const override;

    /**
     * @brief Fetches Gemini models available to the configured account.
     * @param errorMessage Optional destination for provider or transport errors.
     * @return Available models converted to application metadata.
     */
    QVector<AiModelInfo> availableModels(QString* errorMessage = nullptr) const override;

    /**
     * @brief Sends a chat request to Gemini.
     * @param request Provider-neutral chat payload.
     * @return Parsed assistant response or failure details.
     */
    AiChatResponse sendChat(const AiChatRequest& request) const override;

private:
    /** @brief Converts display/provider model names into Gemini API ids. */
    QString normalizedModelId(const QString& modelName) const;
    /** @brief Serializes an application request into Gemini's request body. */
    QByteArray buildRequestBody(const AiChatRequest& request) const;
    /** @brief Extracts the assistant answer and status from Gemini JSON. */
    AiChatResponse parseResponse(const QByteArray& responseBody) const;
    /** @brief Resolves request-specific model override against service defaults. */
    QString modelForRequest(const AiChatRequest& request) const;

private:
    AiServiceSettings m_settings;
    std::shared_ptr<IAiServerClient> m_serverClient;
};
