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
    /** @brief Sets warning title, message, and confirmation button labels. */
    void configureConfirmation(
        const QString& title,
        const QString& message,
        const QString& continueText,
        const QString& cancelText);

private:
    QPushButton* m_okButton{nullptr};
    QPushButton* m_cancelButton{nullptr};
};
