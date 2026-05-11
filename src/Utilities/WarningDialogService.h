#pragma once

#include "Errors/IErrorPresenter.h"
#include "Utilities/IWarningDialogService.h"

#include <QString>

class QWidget;

/**
 * @brief Qt implementation of warning/error dialog services.
 *
 * Responsibilities:
 * - Present warning messages and AppError payloads using standard dialogs.
 * - Satisfy both warning-service and error-presenter interfaces.
 */
class WarningDialogService final : public IWarningDialogService, public IErrorPresenter
{
public:
    /** @brief Creates the service with an optional parent widget. */
    explicit WarningDialogService(QWidget* parent = nullptr);

    /** @brief Sets the parent widget used for dialogs. */
    void setParentWidget(QWidget* parent) override;
    /** @brief Shows a warning dialog. */
    void showWarning(const QString& title, const QString& message) const override;
    /** @brief Shows a warning confirmation dialog. */
    bool confirmWarning(
        const QString& title,
        const QString& message,
        const QString& continueText = "Continue",
        const QString& cancelText = "Cancel") const override;
    /** @brief Shows an error dialog from AppError. */
    void showError(const AppError& error) const override;
    /** @brief Presents an AppError through the error presenter interface. */
    void present(const AppError& error) const override;

private:
    QWidget* m_parent{nullptr};
};
