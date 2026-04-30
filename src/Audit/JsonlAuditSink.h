#pragma once

#include "Audit/IAuditSink.h"

#include <QFile>
#include <QString>

class JsonlAuditSink final : public IAuditSink
{
public:
    explicit JsonlAuditSink(QString filePath, qsizetype maxAttributeLength = 2048);

    void record(const AuditEvent& event) override;

private:
    QString sanitizeValue(const QString& value) const;
    static QString severityName(AuditSeverity severity);
    static QString typeName(AuditEventType type);

private:
    QString m_filePath;
    qsizetype m_maxAttributeLength{2048};
};
