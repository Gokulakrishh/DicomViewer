#pragma once

#include "Errors/IErrorPresenter.h"

class QWidget;

class QtErrorDialogPresenter final : public IErrorPresenter
{
public:
    explicit QtErrorDialogPresenter(QWidget* parent = nullptr);

    void setParentWidget(QWidget* parent) override;
    void present(const AppError& error) const override;

private:
    QWidget* m_parent{nullptr};
};
