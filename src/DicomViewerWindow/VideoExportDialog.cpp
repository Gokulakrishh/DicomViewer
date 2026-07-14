#include "DicomViewerWindow/VideoExportDialog.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardPaths>
#include <QVBoxLayout>

#include <algorithm>

VideoExportDialog::VideoExportDialog(Context context, QWidget* parent)
    : AppDialogBase(parent),
      m_context(std::move(context))
{
    setDialogTitleText(QStringLiteral("Export Cine"));
    setDialogMessageText(
        QStringLiteral("Export a selected frame range as derived, non-diagnostic MP4/H.264 video."));
    setMinimumWidth(520);

    auto* formLayout = new QFormLayout();
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_firstFrameSpinBox = new QSpinBox(this);
    m_firstFrameSpinBox->setRange(1, std::max(1, m_context.frameCount));
    m_firstFrameSpinBox->setValue(1);
    formLayout->addRow(QStringLiteral("First frame:"), m_firstFrameSpinBox);

    m_lastFrameSpinBox = new QSpinBox(this);
    m_lastFrameSpinBox->setRange(1, std::max(1, m_context.frameCount));
    m_lastFrameSpinBox->setValue(std::max(1, m_context.frameCount));
    formLayout->addRow(QStringLiteral("Last frame:"), m_lastFrameSpinBox);

    m_framesPerSecondSpinBox = new QDoubleSpinBox(this);
    m_framesPerSecondSpinBox->setRange(1.0, 120.0);
    m_framesPerSecondSpinBox->setDecimals(3);
    m_framesPerSecondSpinBox->setValue(
        std::clamp(m_context.defaultFramesPerSecond, 1.0, 120.0));
    m_framesPerSecondSpinBox->setSuffix(QStringLiteral(" FPS"));
    formLayout->addRow(QStringLiteral("Frame rate:"), m_framesPerSecondSpinBox);

    auto* timingLabel = new QLabel(videoExportTimingSourceName(m_context.timingSource), this);
    formLayout->addRow(QStringLiteral("Timing source:"), timingLabel);

    auto* outputRow = new QWidget(this);
    auto* outputLayout = new QHBoxLayout(outputRow);
    outputLayout->setContentsMargins(0, 0, 0, 0);
    outputLayout->setSpacing(6);
    m_outputPathEdit = new QLineEdit(outputRow);
    const QString moviesPath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    const QString outputDirectory = moviesPath.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
        : moviesPath;
    const QString baseName = m_context.suggestedBaseName.trimmed().isEmpty()
        ? QStringLiteral("cine-export")
        : m_context.suggestedBaseName.trimmed();
    m_outputPathEdit->setText(
        QFileInfo(outputDirectory, baseName + QStringLiteral("-derived.mp4")).absoluteFilePath());
    auto* browseButton = new QPushButton(QStringLiteral("Browse..."), outputRow);
    outputLayout->addWidget(m_outputPathEdit, 1);
    outputLayout->addWidget(browseButton);
    formLayout->addRow(QStringLiteral("Output file:"), outputRow);

    auto* policyLabel = new QLabel(
        QStringLiteral(
            "The video includes a DERIVED - NON-DIAGNOSTIC label. Patient-identifying overlays "
            "are not exported, and the original DICOM file is not modified."),
        this);
    policyLabel->setWordWrap(true);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel,
        this);

    bodyLayout()->addLayout(formLayout);
    bodyLayout()->addWidget(policyLabel);
    bodyLayout()->addWidget(buttons);

    connect(browseButton, &QPushButton::clicked, this, &VideoExportDialog::chooseOutputPath);
    connect(
        m_firstFrameSpinBox,
        &QSpinBox::valueChanged,
        this,
        &VideoExportDialog::synchronizeFrameRange);
    connect(
        m_lastFrameSpinBox,
        &QSpinBox::valueChanged,
        this,
        &VideoExportDialog::synchronizeFrameRange);
    connect(buttons, &QDialogButtonBox::accepted, this, &VideoExportDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &VideoExportDialog::reject);
}

VideoExportRequest VideoExportDialog::request() const
{
    VideoExportRequest request;
    request.outputPath = m_outputPathEdit->text().trimmed();
    if (!request.outputPath.isEmpty() &&
        !request.outputPath.endsWith(QStringLiteral(".mp4"), Qt::CaseInsensitive))
    {
        const QFileInfo outputInfo(request.outputPath);
        const QString baseName = outputInfo.completeBaseName().isEmpty()
            ? QStringLiteral("cine-derived")
            : outputInfo.completeBaseName();
        request.outputPath = outputInfo.dir().filePath(baseName + QStringLiteral(".mp4"));
    }
    request.sourceSopInstanceUid = m_context.sourceSopInstanceUid;
    request.sourceSeriesInstanceUid = m_context.sourceSeriesInstanceUid;
    request.productVersion = m_context.productVersion;
    request.sourceKind = m_context.sourceKind;
    request.firstFrameIndex = m_firstFrameSpinBox->value() - 1;
    request.lastFrameIndex = m_lastFrameSpinBox->value() - 1;
    request.framesPerSecond = m_framesPerSecondSpinBox->value();
    request.timingSource = m_context.timingSource;
    request.derivedNonDiagnosticOutput = true;
    request.includePatientOverlays = false;
    return request;
}

void VideoExportDialog::chooseOutputPath()
{
    const QString selectedPath = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Export Cine"),
        m_outputPathEdit->text(),
        QStringLiteral("MP4 H.264 Video (*.mp4)"));
    if (!selectedPath.isEmpty())
    {
        m_outputPathEdit->setText(selectedPath);
    }
}

void VideoExportDialog::synchronizeFrameRange()
{
    if (m_firstFrameSpinBox->value() > m_lastFrameSpinBox->value())
    {
        if (sender() == m_firstFrameSpinBox)
        {
            m_lastFrameSpinBox->setValue(m_firstFrameSpinBox->value());
        }
        else
        {
            m_firstFrameSpinBox->setValue(m_lastFrameSpinBox->value());
        }
    }
}

void VideoExportDialog::accept()
{
    const VideoExportRequest exportRequest = request();
    if (exportRequest.outputPath.trimmed().isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("Export Cine"), QStringLiteral("Choose an output file."));
        return;
    }
    if (QFileInfo::exists(exportRequest.outputPath))
    {
        const QMessageBox::StandardButton answer = QMessageBox::question(
            this,
            QStringLiteral("Replace Video"),
            QStringLiteral("The selected file already exists. Replace it?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer != QMessageBox::Yes)
        {
            return;
        }
    }

    QDialog::accept();
}
