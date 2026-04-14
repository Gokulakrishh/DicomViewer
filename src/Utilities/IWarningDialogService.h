#pragma once

#include <QString>

class QWidget;

class IWarningDialogService
{
public:
    virtual ~IWarningDialogService() = default;

    virtual void setParentWidget(QWidget* parent) = 0;
    virtual void showWarning(const QString& title, const QString& message) const = 0;
};
