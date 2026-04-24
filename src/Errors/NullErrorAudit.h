#pragma once

#include "Errors/IErrorAudit.h"

class NullErrorAudit final : public IErrorAudit
{
public:
    void record(const AppError& error) override
    {
        (void)error;
    }
};
