#pragma once

#include "Utilities/AppDialogBase.h"

class QPushButton;

/**
 * @brief Standard warning dialog used by application workflows.
 */
class WarningDialog final : public AppDialogBase
{
    Q_OBJECT

public:
    /** @brief Creates the warning dialog. */
    explicit WarningDialog(QWidget* parent = nullptr);

    /** @brief Sets warning title and message. */
    void configure(const QString& title, const QString& message);

private:
    QPushButton* m_okButton{nullptr};
};
