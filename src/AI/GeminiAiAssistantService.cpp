#include "AI/GeminiAiAssistantService.h"

#include "AI/IAiServerClient.h"

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace
{
QString geminiRole(AiChatRole role)
{
    switch (role)
    {
    case AiChatRole::Assistant:
        return "model";
    case AiChatRole::System:
    case AiChatRole::User:
    default:
        return "user";
    }
}
}

GeminiAiAssistantService::GeminiAiAssistantService(
    AiServiceSettings settings,
    std::shared_ptr<IAiServerClient> serverClient)
    : m_settings(std::move(settings)),
      m_serverClient(std::move(serverClient))
{
}

QString GeminiAiAssistantService::providerId() const
{
    return "gemini";
}

bool GeminiAiAssistantService::isConfigured() const
{
    return m_serverClient &&
           !m_settings.baseUrl.trimmed().isEmpty() &&
           !m_settings.apiKey.trimmed().isEmpty() &&
           !m_settings.model.trimmed().isEmpty();
}

QVector<AiModelInfo> GeminiAiAssistantService::availableModels(QString* errorMessage) const
{
    QVector<AiModelInfo> models;
    if (!m_serverClient || m_settings.baseUrl.trimmed().isEmpty() || m_settings.apiKey.trimmed().isEmpty())
    {
        if (errorMessage)
        {
            *errorMessage = "Gemini AI service is not configured.";
        }
        return models;
    }

    const QString requestUrl = QString("%1/v1beta/models").arg(m_settings.baseUrl.trimmed());
    const AiServerResponse serverResponse = m_serverClient->getJson(
        AiServerRequest{
            requestUrl,
            {
                {"x-goog-api-key", m_settings.apiKey}
            },
            {},
            m_settings.requestTimeoutMs});

    if (!serverResponse.success)
    {
        if (errorMessage)
        {
            *errorMessage = serverResponse.errorMessage;
        }
        return models;
    }

    const QJsonDocument document = QJsonDocument::fromJson(serverResponse.body);
    if (!document.isObject())
    {
        if (errorMessage)
        {
            *errorMessage = "Invalid Gemini model list payload.";
        }
        return models;
    }

    const QJsonArray modelsArray = document.object().value("models").toArray();
    for (const auto& modelValue : modelsArray)
    {
        const QJsonObject modelObject = modelValue.toObject();
        const QJsonArray supportedMethods = modelObject.value("supportedGenerationMethods").toArray();

        bool supportsGenerateContent = false;
        for (const auto& methodValue : supportedMethods)
        {
            if (methodValue.toString() == "generateContent")
            {
                supportsGenerateContent = true;
                break;
            }
        }
        if (!supportsGenerateContent)
        {
            continue;
        }

        const QString fullName = modelObject.value("name").toString();
        const QString modelId = normalizedModelId(fullName);
        if (modelId.isEmpty())
        {
            continue;
        }

        const QString displayName = modelObject.value("displayName").toString(modelId);
        const QString lowerName = modelId.toLower();
        const bool supportsVision =
            lowerName.contains("vision") ||
            lowerName.contains("flash") ||
            lowerName.contains("pro");

        models.append({modelId, displayName, supportsVision});
    }

    return models;
}

AiChatResponse GeminiAiAssistantService::sendChat(const AiChatRequest& request) const
{
    AiChatResponse response;
    response.providerName = "Gemini";
    response.modelName = modelForRequest(request);

    if (!isConfigured())
    {
        response.errorMessage = "Gemini AI service is not configured.";
        return response;
    }

    const QString requestUrl =
        QString("%1/v1beta/models/%2:generateContent")
            .arg(m_settings.baseUrl.trimmed(), normalizedModelId(response.modelName));

    const AiServerResponse serverResponse = m_serverClient->postJson(
        AiServerRequest{
            requestUrl,
            {
                {"x-goog-api-key", m_settings.apiKey}
            },
            buildRequestBody(request),
            m_settings.requestTimeoutMs});

    if (!serverResponse.success)
    {
        response.errorMessage = serverResponse.errorMessage.isEmpty()
                                    ? QString("Gemini request failed with status %1").arg(serverResponse.statusCode)
                                    : serverResponse.errorMessage;
        return response;
    }

    return parseResponse(serverResponse.body);
}

QByteArray GeminiAiAssistantService::buildRequestBody(const AiChatRequest& request) const
{
    QJsonArray contentsArray;
    QString systemInstruction;

    for (const auto& message : request.messages)
    {
        if (message.role == AiChatRole::System)
        {
            if (!systemInstruction.isEmpty())
            {
                systemInstruction += "\n\n";
            }
            systemInstruction += message.content;
            continue;
        }

        QJsonArray partsArray;
        if (!message.content.trimmed().isEmpty())
        {
            partsArray.append(QJsonObject{{"text", message.content}});
        }
        for (const auto& attachment : message.imageAttachments)
        {
            if (attachment.data.isEmpty() || attachment.mimeType.trimmed().isEmpty())
            {
                continue;
            }

            partsArray.append(QJsonObject{
                {"inlineData",
                 QJsonObject{
                     {"mimeType", attachment.mimeType},
                     {"data", QString::fromLatin1(attachment.data.toBase64())}}}});
        }

        QJsonObject contentObject;
        contentObject.insert("role", geminiRole(message.role));
        contentObject.insert("parts", partsArray);
        contentsArray.append(contentObject);
    }

    QJsonObject generationConfig;
    generationConfig.insert(
        "maxOutputTokens",
        request.generationOptions.maxOutputTokens > 0
            ? request.generationOptions.maxOutputTokens
            : m_settings.maxOutputTokens);

    QJsonObject requestObject;
    requestObject.insert("contents", contentsArray);
    requestObject.insert("generationConfig", generationConfig);

    if (!systemInstruction.trimmed().isEmpty())
    {
        requestObject.insert(
            "systemInstruction",
            QJsonObject{{"parts", QJsonArray{QJsonObject{{"text", systemInstruction}}}}});
    }

    return QJsonDocument(requestObject).toJson(QJsonDocument::Compact);
}

AiChatResponse GeminiAiAssistantService::parseResponse(const QByteArray& responseBody) const
{
    AiChatResponse response;
    response.providerName = "Gemini";
    response.modelName = m_settings.model;

    const QJsonDocument document = QJsonDocument::fromJson(responseBody);
    if (!document.isObject())
    {
        response.errorMessage = "Invalid Gemini response payload.";
        return response;
    }

    const QJsonObject rootObject = document.object();
    const QJsonArray candidatesArray = rootObject.value("candidates").toArray();
    if (candidatesArray.isEmpty())
    {
        response.errorMessage = "Gemini response did not contain any candidates.";
        return response;
    }

    const QJsonObject candidateObject = candidatesArray.at(0).toObject();
    response.finishReason = candidateObject.value("finishReason").toString();
    response.modelName = rootObject.value("modelVersion").toString(m_settings.model);

    const QJsonArray partsArray = candidateObject.value("content").toObject().value("parts").toArray();
    QString answerText;
    for (const auto& partValue : partsArray)
    {
        const QString partText = partValue.toObject().value("text").toString();
        if (!partText.isEmpty())
        {
            if (!answerText.isEmpty())
            {
                answerText += "\n";
            }
            answerText += partText;
        }
    }

    response.success = !answerText.trimmed().isEmpty();
    response.answer = answerText.trimmed();
    if (!response.success)
    {
        response.errorMessage = "Gemini response did not contain any text content.";
    }
    return response;
}

QString GeminiAiAssistantService::modelForRequest(const AiChatRequest& request) const
{
    return request.generationOptions.modelOverride.trimmed().isEmpty()
               ? m_settings.model
               : request.generationOptions.modelOverride.trimmed();
}

QString GeminiAiAssistantService::normalizedModelId(const QString& modelName) const
{
    const QString trimmedName = modelName.trimmed();
    return trimmedName.startsWith("models/") ? trimmedName.mid(QStringLiteral("models/").size()) : trimmedName;
}
