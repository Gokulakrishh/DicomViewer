#include "DicomViewerWindow/AnnotationSliceBrowserPanel.h"

#include "DicomViewerWindow/AnnotationUiOptions.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

namespace
{
QString valueOrFallback(const QString& value, const QString& fallback)
{
    const QString trimmedValue = value.trimmed();
    return trimmedValue.isEmpty() ? fallback : trimmedValue;
}

QString sliceTitle(const AnnotationSliceGroup& group)
{
    const QString instanceNumber = group.instanceNumber.trimmed();
    const QString sliceText = instanceNumber.isEmpty() ? QString("Slice") : QString("Slice %1").arg(instanceNumber);
    const QString frameText = group.frameIndex > 0 ? QString(" | Frame %1").arg(group.frameIndex + 1) : QString();
    return QString("%1%2 | %3 annotations").arg(sliceText, frameText).arg(group.rows.size());
}

QString groupKey(const AnnotationSliceGroup& group)
{
    return group.seriesInstanceUid + "|" + group.sopInstanceUid + "|" + QString::number(group.frameIndex);
}

QString typeBreakdown(const AnnotationReportRows& rows)
{
    int distanceCount = 0;
    int polylineCount = 0;
    int angleCount = 0;
    int roiCount = 0;

    for (const AnnotationReportRow& row : rows)
    {
        switch (row.measurementType)
        {
        case MeasurementType::Distance:
            ++distanceCount;
            break;
        case MeasurementType::Polyline:
            ++polylineCount;
            break;
        case MeasurementType::Angle:
            ++angleCount;
            break;
        case MeasurementType::RectangleRoi:
            ++roiCount;
            break;
        }
    }

    QStringList parts;
    if (distanceCount > 0)
    {
        parts.append(QString("Distance %1").arg(distanceCount));
    }
    if (polylineCount > 0)
    {
        parts.append(QString("Polyline %1").arg(polylineCount));
    }
    if (angleCount > 0)
    {
        parts.append(QString("Angle %1").arg(angleCount));
    }
    if (roiCount > 0)
    {
        parts.append(QString("ROI %1").arg(roiCount));
    }

    return parts.join(" | ");
}
}

AnnotationSliceBrowserPanel::AnnotationSliceBrowserPanel(QWidget* parent)
    : QWidget(parent)
{
    buildUi();
}

AnnotationReportFilter AnnotationSliceBrowserPanel::currentFilter() const
{
    AnnotationReportFilter filter;
    filter.searchText = m_searchLineEdit ? m_searchLineEdit->text() : QString();
    filter.bodyRegion = m_regionFilterComboBox ? m_regionFilterComboBox->currentText() : QString();
    filter.measurementType = m_typeFilterComboBox ? annotationMeasurementTypeFilterValue(m_typeFilterComboBox->currentIndex()) : QString("all");
    filter.limit = 200;
    return filter;
}

void AnnotationSliceBrowserPanel::setGroups(const AnnotationSliceGroups& groups)
{
    clearGroups();
    QSet<QString> visibleGroupKeys;

    if (groups.isEmpty())
    {
        m_expandedGroupKeys.clear();
        auto* emptyLabel = new QLabel("No matching annotated slices.", m_groupsWidget);
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setMinimumHeight(48);
        m_groupsLayout->addWidget(emptyLabel);
        m_groupsLayout->addStretch();
        return;
    }

    for (const AnnotationSliceGroup& group : groups)
    {
        visibleGroupKeys.insert(groupKey(group));
        appendGroupCard(group);
    }
    m_expandedGroupKeys.intersect(visibleGroupKeys);
    m_groupsLayout->addStretch();
}

void AnnotationSliceBrowserPanel::buildUi()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(8);

    auto* titleLabel = new QLabel("Annotated Slices", this);
    titleLabel->setStyleSheet("font-weight: 700;");
    rootLayout->addWidget(titleLabel);

    m_searchLineEdit = new QLineEdit(this);
    m_searchLineEdit->setPlaceholderText("Search annotations, slice, patient, study, series...");
    rootLayout->addWidget(m_searchLineEdit);

    auto* filterLayout = new QHBoxLayout();
    filterLayout->setContentsMargins(0, 0, 0, 0);
    filterLayout->setSpacing(6);

    m_regionFilterComboBox = new QComboBox(this);
    m_regionFilterComboBox->addItem("All");
    m_regionFilterComboBox->addItems(annotationBodyRegionOptions());
    filterLayout->addWidget(m_regionFilterComboBox, 1);

    m_typeFilterComboBox = new QComboBox(this);
    m_typeFilterComboBox->addItems({"All types", "Distance", "Polyline", "Angle", "ROI"});
    filterLayout->addWidget(m_typeFilterComboBox, 1);
    rootLayout->addLayout(filterLayout);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    m_groupsWidget = new QWidget(scrollArea);
    m_groupsLayout = new QVBoxLayout(m_groupsWidget);
    m_groupsLayout->setContentsMargins(0, 0, 0, 0);
    m_groupsLayout->setSpacing(6);
    scrollArea->setWidget(m_groupsWidget);
    rootLayout->addWidget(scrollArea, 1);

    connect(m_searchLineEdit, &QLineEdit::textChanged, this, &AnnotationSliceBrowserPanel::filterChanged);
    connect(m_regionFilterComboBox, &QComboBox::currentTextChanged, this, &AnnotationSliceBrowserPanel::filterChanged);
    connect(m_typeFilterComboBox, &QComboBox::currentIndexChanged, this, &AnnotationSliceBrowserPanel::filterChanged);
}

void AnnotationSliceBrowserPanel::clearGroups()
{
    if (!m_groupsLayout)
    {
        return;
    }

    while (QLayoutItem* item = m_groupsLayout->takeAt(0))
    {
        if (QWidget* widget = item->widget())
        {
            widget->deleteLater();
        }
        delete item;
    }
}

void AnnotationSliceBrowserPanel::appendGroupCard(const AnnotationSliceGroup& group)
{
    auto* frame = new QFrame(m_groupsWidget);
    frame->setFrameShape(QFrame::StyledPanel);

    auto* cardLayout = new QVBoxLayout(frame);
    cardLayout->setContentsMargins(6, 4, 6, 4);
    cardLayout->setSpacing(4);

    auto* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(6);

    const QString key = groupKey(group);
    const bool expanded = m_expandedGroupKeys.contains(key);

    auto* expandButton = new QToolButton(frame);
    expandButton->setAutoRaise(true);
    expandButton->setCheckable(true);
    expandButton->setChecked(expanded);
    expandButton->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
    expandButton->setToolTip("Show annotation details");
    headerLayout->addWidget(expandButton);

    auto* titleLabel = new QLabel(sliceTitle(group), frame);
    titleLabel->setStyleSheet("font-weight: 600;");
    titleLabel->setWordWrap(true);
    headerLayout->addWidget(titleLabel, 1);

    auto* goButton = new QPushButton("Go", frame);
    headerLayout->addWidget(goButton);
    cardLayout->addLayout(headerLayout);

    auto* detailsWidget = new QWidget(frame);
    detailsWidget->setVisible(expanded);
    auto* detailsLayout = new QVBoxLayout(detailsWidget);
    detailsLayout->setContentsMargins(24, 2, 0, 4);
    detailsLayout->setSpacing(4);

    auto* contextLabel = new QLabel(
        QString("%1 | %2 | %3")
            .arg(valueOrFallback(group.modality, "Modality"),
                 valueOrFallback(group.seriesDescription, "Series"),
                 valueOrFallback(group.patientName, "Patient")),
        detailsWidget);
    contextLabel->setWordWrap(true);
    detailsLayout->addWidget(contextLabel);

    const QString breakdown = typeBreakdown(group.rows);
    if (!breakdown.isEmpty())
    {
        auto* breakdownLabel = new QLabel(breakdown, detailsWidget);
        breakdownLabel->setWordWrap(true);
        detailsLayout->addWidget(breakdownLabel);
    }

    for (const AnnotationReportRow& row : group.rows)
    {
        auto* rowLabel = new QLabel(
            QString("%1: %2 | %3")
                .arg(row.measurementTypeName,
                     valueOrFallback(row.label, "Unnamed annotation"),
                     valueOrFallback(row.displayValue, "value pending")),
            detailsWidget);
        rowLabel->setWordWrap(true);
        detailsLayout->addWidget(rowLabel);
    }
    cardLayout->addWidget(detailsWidget);

    connect(goButton, &QPushButton::clicked, this, [this, group]() {
        const QString annotationId = group.rows.isEmpty() ? QString() : group.rows.first().annotationId;
        emit goToSliceRequested(group.seriesInstanceUid, group.sopInstanceUid, group.frameIndex, annotationId);
    });
    connect(expandButton, &QToolButton::toggled, this, [this, key, expandButton, detailsWidget](bool checked) {
        expandButton->setArrowType(checked ? Qt::DownArrow : Qt::RightArrow);
        detailsWidget->setVisible(checked);
        if (checked)
        {
            m_expandedGroupKeys.insert(key);
        }
        else
        {
            m_expandedGroupKeys.remove(key);
        }
        detailsWidget->updateGeometry();
    });

    m_groupsLayout->addWidget(frame);
}
