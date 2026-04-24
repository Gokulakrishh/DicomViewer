#include "Errors/QtErrorDialogPresenter.h"

#include "Utilities/WarningDialog.h"

#include <QWidget>

namespace
{
QString buildTitle(const AppError& error)
{
    if (!error.module.trimmed().isEmpty())
    {
        return error.module.trimmed();
    }
    return "Application Error";
}
}

QtErrorDialogPresenter::QtErrorDialogPresenter(QWidget* parent)
    : m_parent(parent)
{
}

void QtErrorDialogPresenter::setParentWidget(QWidget* parent)
{
    m_parent = parent;
}

void QtErrorDialogPresenter::present(const AppError& error) const
{
    WarningDialog dialog(m_parent);
    dialog.configure(buildTitle(error), error.effectiveUserMessage());
    dialog.exec();
}
