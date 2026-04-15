#include "AI/QtHttpAiServerClient.h"

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

#include <algorithm>

namespace
{
AiServerResponse finishReply(QNetworkReply* reply)
{
    AiServerResponse response;
    response.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    response.body = reply->readAll();
    response.success = reply->error() == QNetworkReply::NoError &&
                       response.statusCode >= 200 &&
                       response.statusCode < 300;
    if (!response.success)
    {
        QString errorMessage = reply->errorString();
        const QString responseText = QString::fromUtf8(response.body).trimmed();
        if (response.statusCode > 0)
        {
            errorMessage = QString("HTTP %1: %2").arg(response.statusCode).arg(errorMessage);
        }
        if (!responseText.isEmpty())
        {
            errorMessage += QString("\n%1").arg(responseText);
        }
        response.errorMessage = errorMessage;
    }
    reply->deleteLater();
    return response;
}

AiServerResponse executeRequest(QNetworkReply* reply, int timeoutMs)
{
    QEventLoop eventLoop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);

    QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
    QObject::connect(&timeoutTimer, &QTimer::timeout, [&eventLoop, reply]() {
        if (reply->isRunning())
        {
            reply->abort();
        }
        eventLoop.quit();
    });

    timeoutTimer.start(std::max(1, timeoutMs));
    eventLoop.exec();
    timeoutTimer.stop();

    return finishReply(reply);
}
}

AiServerResponse QtHttpAiServerClient::getJson(const AiServerRequest& request) const
{
    QNetworkAccessManager networkAccessManager;
    QNetworkRequest networkRequest(QUrl(request.url));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    for (auto it = request.headers.constBegin(); it != request.headers.constEnd(); ++it)
    {
        networkRequest.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }

    QNetworkReply* reply = networkAccessManager.get(networkRequest);
    return executeRequest(reply, request.timeoutMs);
}

AiServerResponse QtHttpAiServerClient::postJson(const AiServerRequest& request) const
{
    QNetworkAccessManager networkAccessManager;
    QNetworkRequest networkRequest(QUrl(request.url));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    for (auto it = request.headers.constBegin(); it != request.headers.constEnd(); ++it)
    {
        networkRequest.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }

    QNetworkReply* reply = networkAccessManager.post(networkRequest, request.body);
    return executeRequest(reply, request.timeoutMs);
}
