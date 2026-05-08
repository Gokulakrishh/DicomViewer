#include "DicomViewerWindow/CurrentSliceAnnotationPanel.h"

#include "DicomViewerWindow/AnnotationUiOptions.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace
{
QString valueOrFallback(const QString& value, const QString& fallback)
{
    const QString trimmedValue = value.trimmed();
    return trimmedValue.isEmpty() ? fallback : trimmedValue;
}

QString sliceLabel(const AnnotationCurrentSliceContext& context)
{
    if (!context.instanceNumber.trimmed().isEmpty())
    {
        const QString frameText = context.frameIndex > 0 ? QString(" / Frame %1").arg(context.frameIndex + 1) : QString();
        return QString("Slice %1%2").arg(context.instanceNumber.trimmed(), frameText);
    }
    if (context.sliceIndex >= 0 && context.sliceCount > 0)
    {
        return QString("Slice %1/%2").arg(context.sliceIndex + 1).arg(context.sliceCount);
    }
    return "Slice";
}

QString annotationSummaryLabel(const AnnotationReportRow& row)
{
    return QString("%1 | %2 | %3")
        .arg(row.measurementTypeName,
             row.displayValue.trimmed().isEmpty() ? QString("value pending") : row.displayValue,
             valueOrFallback(row.bodyRegion, "Other"));
}
}

CurrentSliceAnnotationPanel::CurrentSliceAnnotationPanel(QWidget* parent)
    : QWidget(parent)
{
    buildUi();
}

void CurrentSliceAnnotationPanel::setSliceContext(const AnnotationCurrentSliceContext& context)
{
    m_context = context;
    updateDicomInfo();
    if (m_titleLabel)
    {
        m_titleLabel->setText(contextTitle());
    }
    if (m_subtitleLabel)
    {
        m_subtitleLabel->setText(contextSubtitle());
    }
}

void CurrentSliceAnnotationPanel::setRows(const AnnotationReportRows& rows)
{
    clearRows();

    if (!m_context.hasSlice)
    {
        auto* emptyLabel = new QLabel("No active slice.", m_rowsWidget);
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setMinimumHeight(42);
        m_rowsLayout->addWidget(emptyLabel);
        m_rowsLayout->addStretch();
        return;
    }

    if (rows.isEmpty())
    {
        auto* emptyLabel = new QLabel("No annotations on this slice.", m_rowsWidget);
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setMinimumHeight(42);
        m_rowsLayout->addWidget(emptyLabel);
        m_rowsLayout->addStretch();
        return;
    }

    for (const AnnotationReportRow& row : rows)
    {
        appendRowCard(row);
    }
    m_rowsLayout->addStretch();
}

void CurrentSliceAnnotationPanel::buildUi()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(6);

    m_dicomInfoTitleLabel = new QLabel("DICOM Info", this);
    m_dicomInfoTitleLabel->setStyleSheet("font-weight: 700;");
    rootLayout->addWidget(m_dicomInfoTitleLabel);

    m_dicomInfoLabel = new QLabel(this);
    m_dicomInfoLabel->setWordWrap(true);
    m_dicomInfoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    rootLayout->addWidget(m_dicomInfoLabel);

    QFrame *lineSeperator = new QFrame();
    lineSeperator->setFrameShape(QFrame::HLine);
    lineSeperator->setFrameShadow(QFrame::Sunken);
    rootLayout->addWidget(lineSeperator);

    m_titleLabel = new QLabel("Current Slice", this);
    m_titleLabel->setStyleSheet("font-weight: 700;");
    rootLayout->addWidget(m_titleLabel);

    m_subtitleLabel = new QLabel(this);
    m_subtitleLabel->setWordWrap(true);
    rootLayout->addWidget(m_subtitleLabel);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setMinimumHeight(120);
    scrollArea->setMaximumHeight(240);

    m_rowsWidget = new QWidget(scrollArea);
    m_rowsLayout = new QVBoxLayout(m_rowsWidget);
    m_rowsLayout->setContentsMargins(0, 0, 0, 0);
    m_rowsLayout->setSpacing(6);
    scrollArea->setWidget(m_rowsWidget);
    rootLayout->addWidget(scrollArea);
}

void CurrentSliceAnnotationPanel::clearRows()
{
    if (!m_rowsLayout)
    {
        return;
    }

    while (QLayoutItem* item = m_rowsLayout->takeAt(0))
    {
        if (QWidget* widget = item->widget())
        {
            widget->deleteLater();
        }
        delete item;
    }
}

void CurrentSliceAnnotationPanel::appendRowCard(const AnnotationReportRow& row)
{
    auto* frame = new QFrame(m_rowsWidget);
    frame->setFrameShape(QFrame::StyledPanel);

    auto* cardLayout = new QVBoxLayout(frame);
    cardLayout->setContentsMargins(8, 8, 8, 8);
    cardLayout->setSpacing(6);

    auto* labelEdit = new QLineEdit(row.label, frame);
    labelEdit->setPlaceholderText("Annotation name");
    cardLayout->addWidget(labelEdit);

    auto* detailRow = new QHBoxLayout();
    detailRow->setContentsMargins(0, 0, 0, 0);
    detailRow->setSpacing(6);

    auto* regionComboBox = new QComboBox(frame);
    regionComboBox->addItems(annotationBodyRegionOptions());
    int regionIndex = regionComboBox->findText(valueOrFallback(row.bodyRegion, "Other"));
    if (regionIndex < 0)
    {
        regionComboBox->addItem(valueOrFallback(row.bodyRegion, "Other"));
        regionIndex = regionComboBox->count() - 1;
    }
    regionComboBox->setCurrentIndex(regionIndex);
    detailRow->addWidget(regionComboBox, 1);

    auto* deleteButton = new QPushButton("Delete", frame);
    detailRow->addWidget(deleteButton);
    cardLayout->addLayout(detailRow);

    auto* summaryLabel = new QLabel(annotationSummaryLabel(row), frame);
    summaryLabel->setWordWrap(true);
    cardLayout->addWidget(summaryLabel);

    connect(labelEdit, &QLineEdit::editingFinished, this, [this, row, labelEdit, regionComboBox]() {
        emit metadataChanged(row.annotationId, labelEdit->text(), regionComboBox->currentText(), row.seriesInstanceUid);
    });
    connect(regionComboBox, &QComboBox::currentTextChanged, this, [this, row, labelEdit](const QString& bodyRegion) {
        emit metadataChanged(row.annotationId, labelEdit->text(), bodyRegion, row.seriesInstanceUid);
    });
    connect(deleteButton, &QPushButton::clicked, this, [this, row]() {
        emit deleteRequested(row.annotationId, row.seriesInstanceUid, row.sopInstanceUid, row.frameIndex);
    });

    m_rowsLayout->addWidget(frame);
}

void CurrentSliceAnnotationPanel::updateDicomInfo()
{
    if (!m_dicomInfoLabel)
    {
        return;
    }

    m_dicomInfoLabel->setText(dicomInfoText());
}

QString CurrentSliceAnnotationPanel::contextTitle() const
{
    if (!m_context.hasSlice)
    {
        return "Current Slice";
    }

    return QString("Current Slice | %1").arg(sliceLabel(m_context));
}

QString CurrentSliceAnnotationPanel::contextSubtitle() const
{
    if (!m_context.hasSlice)
    {
        return "No DICOM slice is active.";
    }
    return QString(" ");
    /*return QString("%1 | %2 | %3")
        .arg(valueOrFallback(m_context.modality, "Modality"),
             valueOrFallback(m_context.seriesDescription, "Series"),
             valueOrFallback(m_context.patientName, "Patient"));*/
}

QString CurrentSliceAnnotationPanel::dicomInfoText() const
{
    if (!m_context.hasSlice)
    {
        return "No DICOM slice is active.";
    }

    QStringList lines;
    lines << QString("Patient: %1").arg(valueOrFallback(m_context.patientName, "Unknown"));
    if (!m_context.patientAge.trimmed().isEmpty())
    {
        lines << QString("Age: %1").arg(m_context.patientAge.trimmed());
    }
    if (!m_context.patientDob.trimmed().isEmpty())
    {
        lines << QString("DOB: %1").arg(m_context.patientDob.trimmed());
    }
    if (!m_context.doctorName.trimmed().isEmpty())
    {
        lines << QString("Doctor: %1").arg(m_context.doctorName.trimmed());
    }
    lines << QString("Modality: %1").arg(valueOrFallback(m_context.modality, "Unknown"));
    if (!m_context.studyDate.trimmed().isEmpty())
    {
        lines << QString("Study date: %1").arg(m_context.studyDate.trimmed());
    }
    if (!m_context.seriesDescription.trimmed().isEmpty())
    {
        lines << QString("Series: %1").arg(m_context.seriesDescription.trimmed());
    }
    //lines << sliceLabel(m_context);

    return lines.join('\n');
}
