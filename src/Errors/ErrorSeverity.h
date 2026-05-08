#pragma once

/**
 * @brief Severity level for application errors.
 *
 * Severity helps the UI, audit layer, and future verification records distinguish
 * informational messages from failures that may block clinical workflows.
 */
enum class ErrorSeverity
{
    Info = 0,
    Warning = 1,
    Recoverable = 2,
    Critical = 3
};
