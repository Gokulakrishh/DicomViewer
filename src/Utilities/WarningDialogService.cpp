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

void WarningDialogService::showError(const AppError& error) const
{
    present(error);
}

void WarningDialogService::present(const AppError& error) const
{
    showWarning(error.module.isEmpty() ? "Application Error" : error.module, error.effectiveUserMessage());
}
