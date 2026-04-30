#include "Audit/AuditService.h"

#include "Audit/IAuditSink.h"

void AuditService::addSink(std::shared_ptr<IAuditSink> sink)
{
    if (sink)
    {
        m_sinks.push_back(std::move(sink));
    }
}

void AuditService::record(AuditEvent event)
{
    if (!event.timestampUtc.isValid())
    {
        event.timestampUtc = QDateTime::currentDateTimeUtc();
    }

    if (event.eventId.trimmed().isEmpty())
    {
        event.eventId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    for (const auto& sink : m_sinks)
    {
        if (sink)
        {
            sink->record(event);
        }
    }
}
