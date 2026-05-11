#include "Utilities/WarningDialogService.h"

#include "Errors/AppError.h"
#include "Utilities/WarningDialog.h"

#include <QWidget>

WarningDialogService::WarningDialogService(QWidget* parent)
    : m_parent(parent)
{
}

void WarningDialogService::setParentWidget(QWidget* parent)
{
    m_parent = parent;
}

void WarningDialogService::showWarning(const QString& title, const QString& message) const
{
    WarningDialog dialog(m_parent);
    dialog.configure(title, message);
    dialog.exec();
}

bool WarningDialogService::confirmWarning(
    const QString& title,
    const QString& message,
    const QString& continueText,
    const QString& cancelText) const
{
    WarningDialog dialog(m_parent);
    dialog.configureConfirmation(title, message, continueText, cancelText);
    return dialog.exec() == QDialog::Accepted;
}

void WarningDialogService::showError(const AppError& error) const
{
    present(error);
}

void WarningDialogService::present(const AppError& error) const
{
    showWarning(error.module.isEmpty() ? "Application Error" : error.module, error.effectiveUserMessage());
}
