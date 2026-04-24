#pragma once

#include "Errors/ErrorCode.h"
#include "Errors/ErrorSeverity.h"

#include <QString>

struct AppError
{
    ErrorCode code{ErrorCode::Unknown};
    ErrorSeverity severity{ErrorSeverity::Recoverable};
    QString module;
    QString technicalMessage;
    QString userMessage;

    [[nodiscard]] QString effectiveUserMessage() const
    {
        return userMessage.isEmpty() ? technicalMessage : userMessage;
    }
};
