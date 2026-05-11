#include "Utilities/WarningDialog.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

WarningDialog::WarningDialog(QWidget* parent)
    : AppDialogBase(parent)
{
    auto* buttonsLayout = new QHBoxLayout();
    buttonsLayout->addStretch();

    m_cancelButton = new QPushButton("Cancel", this);
    m_cancelButton->hide();
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonsLayout->addWidget(m_cancelButton);

    m_okButton = new QPushButton("OK", this);
    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonsLayout->addWidget(m_okButton);

    bodyLayout()->addLayout(buttonsLayout);
}

void WarningDialog::configure(const QString& title, const QString& message)
{
    setDialogTitleText(title);
    setDialogMessageText(message);
    m_okButton->setText("OK");
    m_cancelButton->hide();
}

void WarningDialog::configureConfirmation(
    const QString& title,
    const QString& message,
    const QString& continueText,
    const QString& cancelText)
{
    setDialogTitleText(title);
    setDialogMessageText(message);
    m_okButton->setText(continueText.trimmed().isEmpty() ? "Continue" : continueText);
    m_cancelButton->setText(cancelText.trimmed().isEmpty() ? "Cancel" : cancelText);
    m_cancelButton->show();
}
