#pragma once

#include "Utilities/AppDialogBase.h"

class QLineEdit;
class QPushButton;

/**
 * @brief Dialog for entering the optional AI provider API key.
 *
 * Responsibilities:
 * - Collect API key text from the user.
 * - Avoid coupling the dialog to the storage mechanism.
 */
class AiApiKeyDialog final : public AppDialogBase
{
    Q_OBJECT

public:
    /** @brief Creates the API key dialog. */
    explicit AiApiKeyDialog(QWidget* parent = nullptr);

    /** @brief Sets the current API key text. */
    void setApiKey(const QString& apiKey);
    /** @brief Returns the entered API key text. */
    QString apiKey() const;

private:
    QLineEdit* m_apiKeyEdit{nullptr};
    QPushButton* m_saveButton{nullptr};
    QPushButton* m_cancelButton{nullptr};
};
