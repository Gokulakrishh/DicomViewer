#pragma once

#include "Errors/IErrorAudit.h"

class IAuditService;

class ErrorAuditAdapter final : public IErrorAudit
{
public:
    explicit ErrorAuditAdapter(IAuditService& auditService);

    void record(const AppError& error) override;

private:
    IAuditService& m_auditService;
};
