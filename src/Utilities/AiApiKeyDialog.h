#pragma once

#include "Utilities/AppDialogBase.h"

class QLineEdit;
class QPushButton;

class AiApiKeyDialog final : public AppDialogBase
{
    Q_OBJECT

public:
    explicit AiApiKeyDialog(QWidget* parent = nullptr);

    void setApiKey(const QString& apiKey);
    QString apiKey() const;

private:
    QLineEdit* m_apiKeyEdit{nullptr};
    QPushButton* m_saveButton{nullptr};
    QPushButton* m_cancelButton{nullptr};
};
