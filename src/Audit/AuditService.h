#pragma once

#include "Audit/IAuditService.h"

#include <memory>
#include <vector>

class IAuditSink;

/**
 * @brief Default audit dispatcher used by the application.
 *
 * Responsibilities:
 * - Maintain configured audit sinks.
 * - Fan out each event to all sinks.
 *
 * Assumptions:
 * - Sinks are lightweight and owned externally through shared pointers.
 * - Recording should remain best-effort to avoid blocking core viewer use.
 */
class AuditService final : public IAuditService
{
public:
    /**
     * @brief Adds an audit sink to the dispatcher.
     * @param sink Sink that receives future audit events.
     */
    void addSink(std::shared_ptr<IAuditSink> sink);

    /**
     * @brief Records an event through every configured sink.
     * @param event Event payload to dispatch.
     */
    void record(AuditEvent event) override;

private:
    std::vector<std::shared_ptr<IAuditSink>> m_sinks;
};
