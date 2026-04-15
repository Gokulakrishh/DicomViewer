#include "Utilities/AppDialogBase.h"

#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>

AppDialogBase::AppDialogBase(QWidget* parent)
    : QDialog(parent)
{
    setModal(true);
    setMinimumWidth(360);
    setSizeGripEnabled(false);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    m_brandLabel = new QLabel("DicomViewer", this);
    m_brandLabel->setObjectName("appDialogBrandLabel");

    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("appDialogTitleLabel");
    m_titleLabel->setWordWrap(true);

    m_messageLabel = new QLabel(this);
    m_messageLabel->setObjectName("appDialogMessageLabel");
    m_messageLabel->setWordWrap(true);

    auto* separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);

    auto* bodyContainer = new QWidget(this);
    m_bodyLayout = new QVBoxLayout(bodyContainer);
    m_bodyLayout->setContentsMargins(0, 0, 0, 0);
    m_bodyLayout->setSpacing(12);

    rootLayout->addWidget(m_brandLabel);
    rootLayout->addWidget(m_titleLabel);
    rootLayout->addWidget(m_messageLabel);
    rootLayout->addWidget(separator);
    rootLayout->addWidget(bodyContainer);

}

void AppDialogBase::setDialogTitleText(const QString& title)
{
    setWindowTitle(title);
    if (m_titleLabel)
    {
        m_titleLabel->setText(title);
    }
}

void AppDialogBase::setDialogMessageText(const QString& message)
{
    if (m_messageLabel)
    {
        m_messageLabel->setText(message);
    }
}

QVBoxLayout* AppDialogBase::bodyLayout() const
{
    return m_bodyLayout;
}
