#pragma once

#include <QDateTime>
#include <QMap>
#include <QString>
#include <QUuid>

enum class AuditSeverity
{
    Info,
    Warning,
    Error,
    Critical
};

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
