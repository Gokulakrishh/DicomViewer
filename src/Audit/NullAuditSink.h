#pragma once

#include "Audit/IAuditSink.h"

/**
 * @brief Audit sink that intentionally discards events.
 *
 * Responsibilities:
 * - Provide a no-op sink for tests or builds where persistence is unavailable.
 *
 * Assumptions:
 * - This class must not be used as the only audit sink in regulated workflows.
 */
class NullAuditSink final : public IAuditSink
{
public:
    /**
     * @brief Accepts an event without persisting it.
     * @param event Event intentionally ignored.
     */
    void record(const AuditEvent& event) override
    {
        (void)event;
    }
};
