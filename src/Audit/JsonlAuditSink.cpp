#include "Audit/JsonlAuditSink.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

namespace
{
constexpr int kAuditSchemaVersion = 1;
}

JsonlAuditSink::JsonlAuditSink(QString filePath, qsizetype maxAttributeLength)
    : m_filePath(std::move(filePath)),
      m_maxAttributeLength(maxAttributeLength)
{
}

void JsonlAuditSink::record(const AuditEvent& event)
{
    const QFileInfo fileInfo(m_filePath);
    QDir directory(fileInfo.absolutePath());
    if (!directory.exists())
    {
        directory.mkpath(".");
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        return;
    }

    QJsonObject attributes;
    for (auto it = event.attributes.cbegin(); it != event.attributes.cend(); ++it)
    {
        attributes.insert(it.key(), sanitizeValue(it.value()));
    }

    QJsonObject object;
    object.insert("schema_version", kAuditSchemaVersion);
    object.insert("event_id", event.eventId);
    object.insert("timestamp_utc", event.timestampUtc.toUTC().toString(Qt::ISODateWithMs));
    object.insert("type", typeName(event.type));
    object.insert("severity", severityName(event.severity));
    object.insert("module", sanitizeValue(event.module));
    object.insert("action", sanitizeValue(event.action));
    object.insert("subject_id", sanitizeValue(event.subjectId));
    object.insert("message", sanitizeValue(event.message));
    object.insert("attributes", attributes);

    QTextStream stream(&file);
    stream << QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact)) << '\n';
}

QString JsonlAuditSink::sanitizeValue(const QString& value) const
{
    QString sanitized = value;
    sanitized.replace('\n', ' ');
    sanitized.replace('\r', ' ');
    sanitized = sanitized.trimmed();
    if (m_maxAttributeLength > 0 && sanitized.size() > m_maxAttributeLength)
    {
        sanitized = sanitized.left(m_maxAttributeLength) + "...[truncated]";
    }

    return sanitized;
}

QString JsonlAuditSink::severityName(AuditSeverity severity)
{
    switch (severity)
    {
    case AuditSeverity::Info:
        return "info";
    case AuditSeverity::Warning:
        return "warning";
    case AuditSeverity::Error:
        return "error";
    case AuditSeverity::Critical:
        return "critical";
    }

    return "error";
}

QString JsonlAuditSink::typeName(AuditEventType type)
{
    switch (type)
    {
    case AuditEventType::SoftwareLifecycle:
        return "software_lifecycle";
    case AuditEventType::UserAction:
        return "user_action";
    case AuditEventType::DataImport:
        return "data_import";
    case AuditEventType::DataValidation:
        return "data_validation";
    case AuditEventType::ClinicalOperation:
        return "clinical_operation";
    case AuditEventType::ConfigurationChange:
        return "configuration_change";
    case AuditEventType::Error:
        return "error";
    }

    return "software_lifecycle";
}
