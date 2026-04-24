#pragma once

#include "Errors/IErrorPresenter.h"
#include "Utilities/IWarningDialogService.h"

#include <QString>

class QWidget;

class WarningDialogService final : public IWarningDialogService, public IErrorPresenter
{
public:
    explicit WarningDialogService(QWidget* parent = nullptr);

    void setParentWidget(QWidget* parent) override;
    void showWarning(const QString& title, const QString& message) const override;
    void showError(const AppError& error) const override;
    void present(const AppError& error) const override;

private:
    QWidget* m_parent{nullptr};
};
