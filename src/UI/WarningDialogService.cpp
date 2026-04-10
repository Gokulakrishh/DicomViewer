#include "UI/WarningDialogService.h"

#include <QMessageBox>
#include <QWidget>

WarningDialogService::WarningDialogService(QWidget* parent)
    : m_parent(parent)
{
}

void WarningDialogService::showWarning(const QString& title, const QString& message) const
{
    QMessageBox::warning(m_parent, title, message);
}
