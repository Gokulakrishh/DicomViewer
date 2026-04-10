#pragma once

#include <QString>

class QWidget;

class WarningDialogService
{
public:
    explicit WarningDialogService(QWidget* parent = nullptr);

    void showWarning(const QString& title, const QString& message) const;

private:
    QWidget* m_parent{nullptr};
};
