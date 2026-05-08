#pragma once

#include "Errors/IErrorAudit.h"

class IAuditService;

/**
 * @brief Bridges application error reporting into the audit layer.
 *
 * Responsibilities:
 * - Convert AppError instances into auditable events.
 * - Keep error producers independent of the concrete audit service.
 *
 * Assumptions:
 * - Error audit entries support traceability and investigation, not automatic
 *   clinical safety decisions.
 */
class ErrorAuditAdapter final : public IErrorAudit
{
public:
    /**
     * @brief Creates an adapter bound to an audit service.
     * @param auditService Audit service that receives converted errors.
     */
    explicit ErrorAuditAdapter(IAuditService& auditService);

    /**
     * @brief Records an application error through the audit service.
     * @param error Error to convert and record.
     */
    void record(const AppError& error) override;

private:
    IAuditService& m_auditService;
};
