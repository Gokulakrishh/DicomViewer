#pragma once

#include <QByteArray>
#include <QMap>
#include <QString>

struct AiServerRequest
{
    QString url;
    QMap<QString, QString> headers;
    QByteArray body;
    int timeoutMs{30000};
};

struct AiServerResponse
{
    bool success{false};
    int statusCode{0};
    QByteArray body;
    QString errorMessage;
};

class IAiServerClient
{
public:
    virtual ~IAiServerClient() = default;

    virtual AiServerResponse getJson(const AiServerRequest& request) const = 0;
    virtual AiServerResponse postJson(const AiServerRequest& request) const = 0;
};
