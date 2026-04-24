#pragma once

#include "Errors/AppError.h"

class QWidget;

class IErrorPresenter
{
public:
    virtual ~IErrorPresenter() = default;

    virtual void setParentWidget(QWidget* parent) = 0;
    virtual void present(const AppError& error) const = 0;
};
