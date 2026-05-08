#pragma once

#include "Audit/AuditEvent.h"

/**
 * @brief Persistence boundary for audit events.
 *
 * Responsibilities:
 * - Accept normalized audit events from the application.
 * - Hide storage format decisions such as JSONL, database, or future remote
 *   sinks.
 *
 * Assumptions:
 * - Implementations should avoid throwing from record paths used during
 *   clinical workflows.
 */
class IAuditSink
{
public:
    virtual ~IAuditSink() = default;

    /**
     * @brief Persists or forwards an audit event.
     * @param event Normalized audit event to record.
     */
    virtual void record(const AuditEvent& event) = 0;
};
