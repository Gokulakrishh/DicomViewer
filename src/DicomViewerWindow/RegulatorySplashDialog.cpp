#include "DicomViewerWindow/RegulatorySplashDialog.h"

#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QVBoxLayout>

#include "AppVersion.h"
#include "Utilities/AppIcons.h"

RegulatorySplashDialog::RegulatorySplashDialog(QWidget* parent)
    : QDialog(parent)
{
    setModal(true);
    setWindowTitle(QString::fromUtf8(AppVersion::kDisplayName));
    setWindowIcon(AppIcons::applicationIcon());
    setSizeGripEnabled(false);
    setMinimumWidth(600);

    buildUi();
    updateBranding();
    updateText();
}

void RegulatorySplashDialog::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 22, 24, 18);
    root->setSpacing(12);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setWordWrap(true);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    {
        QFont f = m_titleLabel->font();
        f.setPointSizeF(f.pointSizeF() + 4.0);
        f.setBold(true);
        m_titleLabel->setFont(f);
    }
    m_titleLabel->setStyleSheet("QLabel { color: rgba(255,255,255,160); background: transparent;}");

    m_logoLabel = new QLabel(this);
    m_logoLabel->setFixedSize(300, 300);
    m_logoLabel->setAlignment(Qt::AlignCenter);
    m_logoLabel->setStyleSheet("QLabel { background: transparent; }");

    m_bodyLabel = new QLabel(this);
    m_bodyLabel->setWordWrap(true);
    m_bodyLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_bodyLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_bodyLabel->setStyleSheet("QLabel { color: rgba(255,255,255,210); background: transparent; }");

    auto* footerRow = new QHBoxLayout();
    footerRow->setSpacing(10);
    footerRow->addStretch(1);

    m_okButton = new QPushButton(tr("OK"), this);
    m_okButton->setDefault(true);
    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
    footerRow->addWidget(m_okButton);

    root->addWidget(m_titleLabel);
    root->addWidget(m_logoLabel, 0, Qt::AlignHCenter);
    root->addWidget(m_bodyLabel);
    root->addItem(new QSpacerItem(0, 6, QSizePolicy::Minimum, QSizePolicy::Expanding));
    root->addLayout(footerRow);
}

void RegulatorySplashDialog::updateBranding()
{
    const QPixmap logo = AppIcons::logoPixmap(m_logoLabel->size());
    if (!logo.isNull()) {
        m_logoLabel->setPixmap(logo);
    }

    m_titleLabel->setText(
        QString("%1\nVersion %2")
            .arg(QString::fromUtf8(AppVersion::kDisplayName),
                 QString::fromUtf8(AppVersion::kVersionString)));
}

void RegulatorySplashDialog::updateText()
{
    m_bodyLabel->setText(
        "Software Use and Regulatory Information\n\n"
        "This software is intended solely for educational and research purposes, including "
        "DICOM viewing, software development, workflow evaluation, and technical review.\n\n"
        "This software is not cleared, approved, or certified as a medical device. It must "
        "not be used for clinical diagnosis, treatment planning, patient management, or any "
        "other clinical decision-making.\n\n"
        "Audit, quality management, verification, validation, and regulatory approval "
        "activities are in progress.\n\n"
        "By clicking OK, you acknowledge that you understand this software is not for "
        "diagnostic or treatment use.");
}
