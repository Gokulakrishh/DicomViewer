#include "Utilities/WarningDialog.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

WarningDialog::WarningDialog(QWidget* parent)
    : AppDialogBase(parent)
{
    auto* buttonsLayout = new QHBoxLayout();
    buttonsLayout->addStretch();

    m_okButton = new QPushButton("OK", this);
    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonsLayout->addWidget(m_okButton);

    bodyLayout()->addLayout(buttonsLayout);
}

void WarningDialog::configure(const QString& title, const QString& message)
{
    setDialogTitleText(title);
    setDialogMessageText(message);
}
