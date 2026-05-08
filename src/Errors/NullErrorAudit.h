#pragma once

#include "Errors/IErrorAudit.h"

/**
 * @brief Error audit implementation that intentionally discards errors.
 *
 * Responsibilities:
 * - Provide a no-op audit dependency for tests or non-audited builds.
 *
 * Assumptions:
 * - Regulated workflows should use a real audit implementation instead.
 */
class NullErrorAudit final : public IErrorAudit
{
public:
    /**
     * @brief Accepts an error without recording it.
     * @param error Error intentionally ignored.
     */
    void record(const AppError& error) override
    {
        (void)error;
    }
};
