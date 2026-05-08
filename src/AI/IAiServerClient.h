#pragma once

#include <QByteArray>
#include <QMap>
#include <QString>

/**
 * @brief HTTP request envelope for AI provider communication.
 *
 * The request body is already serialized by the assistant service so the client
 * can remain a small transport abstraction.
 */
struct AiServerRequest
{
    QString url;
    QMap<QString, QString> headers;
    QByteArray body;
    int timeoutMs{30000};
};

/**
 * @brief HTTP response returned by an AI transport client.
 *
 * Transport errors and non-success HTTP statuses are represented as data so UI
 * code can present failures without exception handling.
 */
struct AiServerResponse
{
    bool success{false};
    int statusCode{0};
    QByteArray body;
    QString errorMessage;
};

/**
 * @brief Minimal HTTP JSON transport interface used by AI services.
 *
 * Responsibilities:
 * - Execute JSON GET and POST requests.
 * - Isolate Qt/network implementation details from provider adapters.
 *
 * Assumptions:
 * - Request bodies are prepared by the caller.
 * - Responses are not interpreted at this layer.
 */
class IAiServerClient
{
public:
    virtual ~IAiServerClient() = default;

    /**
     * @brief Executes a JSON GET request.
     * @param request Fully prepared HTTP request envelope.
     * @return Raw transport response.
     */
    virtual AiServerResponse getJson(const AiServerRequest& request) const = 0;

    /**
     * @brief Executes a JSON POST request.
     * @param request Fully prepared HTTP request envelope.
     * @return Raw transport response.
     */
    virtual AiServerResponse postJson(const AiServerRequest& request) const = 0;
};
