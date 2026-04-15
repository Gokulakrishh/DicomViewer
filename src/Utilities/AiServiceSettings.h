#pragma once

#include <QString>

enum class AiProvider
{
    None = 0,
    Gemini,
    LocalServer
};

enum class AiReasoningLevel
{
    Low = 0,
    Medium,
    High
};

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
