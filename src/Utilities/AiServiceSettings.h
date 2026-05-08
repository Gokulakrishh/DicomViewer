#pragma once

#include <QString>

/**
 * @brief Supported optional AI assistant providers.
 */
enum class AiProvider
{
    None = 0,
    Gemini,
    LocalServer
};

/**
 * @brief User-selectable reasoning budget for AI assistant requests.
 */
enum class AiReasoningLevel
{
    Low = 0,
    Medium,
    High
};

/**
 * @brief Persisted AI assistant configuration.
 *
 * Assumptions:
 * - AI assistance is optional and should not be treated as diagnostic output.
 * - Secrets should move to OS keychain/secrets management before regulated use.
 */
struct AiServiceSettings
{
    AiProvider provider{AiProvider::None};
    QString baseUrl;
    QString apiKey;
    QString model;
    AiReasoningLevel defaultReasoningLevel{AiReasoningLevel::Medium};
    int requestTimeoutMs{30000};
    int maxOutputTokens{2048};
};
