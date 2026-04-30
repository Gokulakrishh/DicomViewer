#pragma once

#include "Audit/IAuditSink.h"

class NullAuditSink final : public IAuditSink
{
public:
    void record(const AuditEvent& event) override
    {
        (void)event;
    }
};
