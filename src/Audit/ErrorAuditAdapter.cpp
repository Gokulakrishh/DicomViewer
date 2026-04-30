#include "Audit/ErrorAuditAdapter.h"

#include "Audit/IAuditService.h"
#include "Errors/AppError.h"

namespace
{
AuditSeverity severityForError(ErrorSeverity severity)
{
    switch (severity)
    {
    case ErrorSeverity::Info:
        return AuditSeverity::Info;
    case ErrorSeverity::Warning:
        return AuditSeverity::Warning;
    case ErrorSeverity::Recoverable:
        return AuditSeverity::Error;
    case ErrorSeverity::Critical:
        return AuditSeverity::Critical;
    }

    return AuditSeverity::Error;
}
}

ErrorAuditAdapter::ErrorAuditAdapter(IAuditService& auditService)
    : m_auditService(auditService)
{
}

void ErrorAuditAdapter::record(const AppError& error)
{
    AuditEvent event;
    event.type = AuditEventType::Error;
    event.severity = severityForError(error.severity);
    event.module = error.module;
    event.action = "AppError";
    event.subjectId = QString::number(static_cast<int>(error.code));
    event.message = error.technicalMessage;
    event.attributes.insert("user_message", error.userMessage);
    event.attributes.insert("error_code", QString::number(static_cast<int>(error.code)));
    m_auditService.record(std::move(event));
}
