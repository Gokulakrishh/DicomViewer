#pragma once

#include "Utilities/IWarningDialogService.h"

#include <QString>

class QWidget;

class WarningDialogService final : public IWarningDialogService
{
public:
    explicit WarningDialogService(QWidget* parent = nullptr);

    void setParentWidget(QWidget* parent) override;
    void showWarning(const QString& title, const QString& message) const override;

private:
    QWidget* m_parent{nullptr};
};
