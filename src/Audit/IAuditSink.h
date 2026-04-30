#pragma once

#include "Audit/AuditEvent.h"

class IAuditSink
{
public:
    virtual ~IAuditSink() = default;

    virtual void record(const AuditEvent& event) = 0;
};
