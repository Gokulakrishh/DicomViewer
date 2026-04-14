#include "Utilities/WarningDialogService.h"

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
