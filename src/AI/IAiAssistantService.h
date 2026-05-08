#pragma once

#include "AI/AiChatTypes.h"

/**
 * @brief Provider-neutral interface for optional AI assistant integrations.
 *
 * Responsibilities:
 * - Expose provider identity and configuration state.
 * - Return available models for UI selection.
 * - Send structured chat requests without leaking provider-specific transport
 *   details into the viewer.
 *
 * Assumptions:
 * - AI assistance is ancillary and must not be treated as diagnostic output.
 * - Implementations should fail gracefully when credentials or network access
 *   are unavailable.
 */
class IAiAssistantService
{
public:
    virtual ~IAiAssistantService() = default;

    /**
     * @brief Returns the stable provider identifier.
     * @return Provider id used for settings and diagnostics.
     */
    virtual QString providerId() const = 0;

    /**
     * @brief Reports whether the provider has enough local configuration to run.
     * @return True when required credentials/settings are present.
     */
    virtual bool isConfigured() const = 0;

    /**
     * @brief Loads models available for this provider.
     * @param errorMessage Optional destination for a recoverable provider error.
     * @return Provider model metadata suitable for UI presentation.
     */
    virtual QVector<AiModelInfo> availableModels(QString* errorMessage = nullptr) const = 0;

    /**
     * @brief Sends a chat request to the configured provider.
     * @param request Provider-neutral conversation payload.
     * @return Assistant response, including error details on failure.
     */
    virtual AiChatResponse sendChat(const AiChatRequest& request) const = 0;
};
