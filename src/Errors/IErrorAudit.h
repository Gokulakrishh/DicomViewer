#pragma once

#include "Errors/AppError.h"

class IErrorAudit
{
public:
    virtual ~IErrorAudit() = default;

    virtual void record(const AppError& error) = 0;
};
