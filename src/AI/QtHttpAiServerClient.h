#pragma once

#include "AI/IAiServerClient.h"

/**
 * @brief Qt-based HTTP transport for AI provider requests.
 *
 * Responsibilities:
 * - Execute blocking JSON HTTP requests through Qt networking primitives.
 * - Return raw success/error data without provider-specific parsing.
 *
 * Assumptions:
 * - Callers run requests off the UI thread when latency could affect
 *   interaction.
 */
class QtHttpAiServerClient final : public IAiServerClient
{
public:
    /**
     * @brief Executes a JSON GET request.
     * @param request Fully prepared request envelope.
     * @return Raw HTTP response.
     */
    AiServerResponse getJson(const AiServerRequest& request) const override;

    /**
     * @brief Executes a JSON POST request.
     * @param request Fully prepared request envelope.
     * @return Raw HTTP response.
     */
    AiServerResponse postJson(const AiServerRequest& request) const override;
};
