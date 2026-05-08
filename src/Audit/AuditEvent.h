#pragma once

#include <QDateTime>
#include <QMap>
#include <QString>
#include <QUuid>

/**
 * @brief Severity assigned to an auditable event.
 *
 * Severity supports filtering and escalation in later QMS/audit review tools.
 */
enum class AuditSeverity
{
    Info,
    Warning,
    Error,
    Critical
};

/**
 * @brief Functional category for an auditable event.
 *
 * Categories are intentionally broad so feature code can remain independent of
 * a concrete audit sink or regulatory report format.
 */
enum class AuditEventType
{
    SoftwareLifecycle,
    UserAction,
    DataImport,
    DataValidation,
    ClinicalOperation,
    ConfigurationChange,
    Error
};

/**
 * @brief Structured audit event captured by the application.
 *
 * Responsibilities:
 * - Carry stable event identity, time, category, severity, and contextual fields.
 * - Preserve additional attributes without forcing each module to define a new
 *   persistence schema.
 *
 * Assumptions:
 * - Timestamps are stored in UTC.
 * - Audit events support traceability work but are not, by themselves, proof of
 *   ISO 13485 or IEC 62304 compliance.
 */
struct AuditEvent
{
    QString eventId{QUuid::createUuid().toString(QUuid::WithoutBraces)};
    QDateTime timestampUtc{QDateTime::currentDateTimeUtc()};
    AuditEventType type{AuditEventType::SoftwareLifecycle};
    AuditSeverity severity{AuditSeverity::Info};
    QString module;
    QString action;
    QString subjectId;
    QString message;
    QMap<QString, QString> attributes;
};
