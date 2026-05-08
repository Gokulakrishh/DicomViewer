#pragma once

#include "Errors/ErrorCode.h"
#include "Errors/ErrorSeverity.h"

#include <QString>

/**
 * @brief Structured application error used across services and UI boundaries.
 *
 * Responsibilities:
 * - Carry a stable error code, severity, and module name.
 * - Separate technical diagnostics from user-facing messages.
 *
 * Assumptions:
 * - Errors can be presented to users and recorded into audit logs without
 *   throwing exceptions through viewer workflows.
 */
struct AppError
{
    ErrorCode code{ErrorCode::Unknown};
    ErrorSeverity severity{ErrorSeverity::Recoverable};
    QString module;
    QString technicalMessage;
    QString userMessage;

    /**
     * @brief Returns the preferred message for UI presentation.
     * @return User message when provided; otherwise the technical message.
     */
    [[nodiscard]] QString effectiveUserMessage() const
    {
        return userMessage.isEmpty() ? technicalMessage : userMessage;
    }
};
