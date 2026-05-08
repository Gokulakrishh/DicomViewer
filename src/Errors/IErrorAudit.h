#pragma once

#include "Errors/AppError.h"

/**
 * @brief Interface for recording structured application errors.
 *
 * Responsibilities:
 * - Decouple error producers from the concrete audit/logging mechanism.
 * - Preserve AppError details for later diagnostics and traceability review.
 */
class IErrorAudit
{
public:
    virtual ~IErrorAudit() = default;

    /**
     * @brief Records an application error.
     * @param error Structured error payload.
     */
    virtual void record(const AppError& error) = 0;
};
