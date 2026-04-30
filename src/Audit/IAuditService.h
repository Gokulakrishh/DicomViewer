#pragma once

#include "Audit/AuditEvent.h"

class IAuditService
{
public:
    virtual ~IAuditService() = default;

    virtual void record(AuditEvent event) = 0;
};
