#include "Utilities/AiApiKeyDialog.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>

AiApiKeyDialog::AiApiKeyDialog(QWidget* parent)
    : AppDialogBase(parent)
{
    setDialogTitleText("AI API Key");
    setDialogMessageText("Enter your personal AI API key. It will be stored in config.ini in encrypted form.");

    auto* formLayout = new QFormLayout();
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(8);

    m_apiKeyEdit = new QLineEdit(this);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setClearButtonEnabled(true);
    formLayout->addRow("API Key", m_apiKeyEdit);

    auto* buttonsLayout = new QHBoxLayout();
    buttonsLayout->addStretch();

    m_cancelButton = new QPushButton("Cancel", this);
    m_saveButton = new QPushButton("Save", this);
    m_saveButton->setObjectName("primaryActionButton");

    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_saveButton, &QPushButton::clicked, this, &QDialog::accept);

    buttonsLayout->addWidget(m_cancelButton);
    buttonsLayout->addWidget(m_saveButton);

    bodyLayout()->addLayout(formLayout);
    bodyLayout()->addLayout(buttonsLayout);
}

void AiApiKeyDialog::setApiKey(const QString& apiKey)
{
    if (m_apiKeyEdit)
    {
        m_apiKeyEdit->setText(apiKey);
    }
}

QString AiApiKeyDialog::apiKey() const
{
    return m_apiKeyEdit ? m_apiKeyEdit->text().trimmed() : QString();
}
