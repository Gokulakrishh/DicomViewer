#pragma once

#include "Errors/AppError.h"

#include <QString>

class QWidget;

/**
 * @brief UI boundary for warning and error dialogs.
 *
 * Responsibilities:
 * - Present user-visible warnings/errors from workflows.
 * - Keep service code independent of concrete dialog classes.
 */
class IWarningDialogService
{
public:
    virtual ~IWarningDialogService() = default;

    /** @brief Sets the parent widget used for dialogs. */
    virtual void setParentWidget(QWidget* parent) = 0;
    /** @brief Shows a warning dialog. */
    virtual void showWarning(const QString& title, const QString& message) const = 0;
    /** @brief Shows an error dialog for an AppError. */
    virtual void showError(const AppError& error) const = 0;
};
