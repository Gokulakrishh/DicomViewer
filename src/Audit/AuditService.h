#pragma once

#include "Audit/IAuditService.h"

#include <memory>
#include <vector>

class IAuditSink;

class AuditService final : public IAuditService
{
public:
    void addSink(std::shared_ptr<IAuditSink> sink);
    void record(AuditEvent event) override;

private:
    std::vector<std::shared_ptr<IAuditSink>> m_sinks;
};
