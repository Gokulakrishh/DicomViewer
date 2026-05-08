#pragma once

#include "Audit/AuditEvent.h"

/**
 * @brief Application-facing audit service interface.
 *
 * Responsibilities:
 * - Provide one recording entry point for feature modules.
 * - Decouple clinical and data-management workflows from audit persistence.
 *
 * Assumptions:
 * - Events may be enriched or routed by concrete implementations.
 */
class IAuditService
{
public:
    virtual ~IAuditService() = default;

    /**
     * @brief Records an auditable application event.
     * @param event Event payload; passed by value so services may normalize it.
     */
    virtual void record(AuditEvent event) = 0;
};
