#pragma once

#include "Utilities/AppDialogBase.h"

class QPushButton;

class WarningDialog final : public AppDialogBase
{
    Q_OBJECT

public:
    explicit WarningDialog(QWidget* parent = nullptr);

    void configure(const QString& title, const QString& message);

private:
    QPushButton* m_okButton{nullptr};
};
