#include "VTK/MPR/Window/VtkMprAnnotationDock.h"

#include "DicomViewerWindow/AnnotationUiOptions.h"
#include "ViewerTools/Measurements/MeasurementService.h"

#include <QComboBox>
#include <QHash>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <algorithm>

namespace
{
constexpr int kAnnotationIdRole = Qt::UserRole + 1;
}

VtkMprAnnotationDock::VtkMprAnnotationDock(QWidget* parent)
    : QWidget(parent)
{
    buildUi();
    setRecords({});
}

void VtkMprAnnotationDock::setRecords(const QList<MprMeasurementAnnotationRecord>& records)
{
    if (!m_treeWidget || !m_summaryLabel)
    {
        return;
    }

    m_treeWidget->clear();
    m_recordsById.clear();
    m_summaryLabel->setText(QString("%1 MPR annotation%2")
        .arg(records.size())
        .arg(records.size() == 1 ? QString() : QStringLiteral("s")));

    QHash<QString, QTreeWidgetItem*> groupItems;
    for (const MprMeasurementAnnotationRecord& record : records)
    {
        if (!record.measurement.id.isEmpty())
        {
            m_recordsById.insert(record.measurement.id, record);
        }

        const QString key = groupKey(record);
        QTreeWidgetItem* groupItem = groupItems.value(key, nullptr);
        if (!groupItem)
        {
            groupItem = new QTreeWidgetItem(m_treeWidget);
            groupItem->setText(0, key);
            groupItem->setFirstColumnSpanned(true);
            groupItem->setExpanded(true);
            groupItems.insert(key, groupItem);
        }

        auto* rowItem = new QTreeWidgetItem(groupItem);
        rowItem->setText(0, record.label.trimmed().isEmpty()
            ? measurementTypeName(record.measurement.type)
            : record.label.trimmed());
        rowItem->setText(1, measurementTypeName(record.measurement.type));
        rowItem->setText(2, displayValue(record));
        rowItem->setData(0, kAnnotationIdRole, record.measurement.id);
    }

    refreshActions();
    refreshEditor();
}

void VtkMprAnnotationDock::buildUi()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(8, 8, 8, 8);
    rootLayout->setSpacing(8);

    auto* titleLabel = new QLabel("MPR Annotations", this);
    titleLabel->setStyleSheet("font-weight: 700;");
    rootLayout->addWidget(titleLabel);

    m_summaryLabel = new QLabel(this);
    rootLayout->addWidget(m_summaryLabel);

    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setColumnCount(3);
    m_treeWidget->setHeaderLabels({"Name", "Type", "Value"});
    m_treeWidget->setRootIsDecorated(true);
    m_treeWidget->setAlternatingRowColors(true);
    m_treeWidget->header()->setStretchLastSection(true);
    m_treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_treeWidget->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    rootLayout->addWidget(m_treeWidget, 1);

    auto* editorLabel = new QLabel("Selected Annotation", this);
    editorLabel->setStyleSheet("font-weight: 700;");
    rootLayout->addWidget(editorLabel);

    m_labelEdit = new QLineEdit(this);
    m_labelEdit->setPlaceholderText("Annotation name");
    rootLayout->addWidget(m_labelEdit);

    m_bodyRegionComboBox = new QComboBox(this);
    m_bodyRegionComboBox->addItems(annotationBodyRegionOptions());
    rootLayout->addWidget(m_bodyRegionComboBox);

    m_noteEdit = new QLineEdit(this);
    m_noteEdit->setPlaceholderText("Note");
    rootLayout->addWidget(m_noteEdit);

    auto* actionLayout = new QHBoxLayout();
    actionLayout->setContentsMargins(0, 0, 0, 0);
    actionLayout->setSpacing(6);

    m_goButton = new QPushButton("Go", this);
    m_applyButton = new QPushButton("Apply", this);
    m_deleteButton = new QPushButton("Delete", this);
    actionLayout->addWidget(m_goButton);
    actionLayout->addWidget(m_applyButton);
    actionLayout->addWidget(m_deleteButton);
    rootLayout->addLayout(actionLayout);

    connect(m_treeWidget, &QTreeWidget::itemSelectionChanged, this, &VtkMprAnnotationDock::refreshActions);
    connect(m_treeWidget, &QTreeWidget::itemSelectionChanged, this, &VtkMprAnnotationDock::refreshEditor);
    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem*, int) {
        const QString annotationId = selectedAnnotationId();
        if (!annotationId.isEmpty())
        {
            emit goToAnnotationRequested(annotationId);
        }
    });
    connect(m_goButton, &QPushButton::clicked, this, [this]() {
        const QString annotationId = selectedAnnotationId();
        if (!annotationId.isEmpty())
        {
            emit goToAnnotationRequested(annotationId);
        }
    });
    connect(m_applyButton, &QPushButton::clicked, this, [this]() {
        const QString annotationId = selectedAnnotationId();
        if (!annotationId.isEmpty() && m_labelEdit && m_bodyRegionComboBox && m_noteEdit)
        {
            emit metadataChanged(
                annotationId,
                m_labelEdit->text(),
                m_bodyRegionComboBox->currentText(),
                m_noteEdit->text());
        }
    });
    connect(m_deleteButton, &QPushButton::clicked, this, [this]() {
        const QString annotationId = selectedAnnotationId();
        if (!annotationId.isEmpty())
        {
            emit deleteAnnotationRequested(annotationId);
        }
    });
}

void VtkMprAnnotationDock::refreshActions()
{
    const bool hasAnnotationSelection = !selectedAnnotationId().isEmpty();
    if (m_goButton)
    {
        m_goButton->setEnabled(hasAnnotationSelection);
    }
    if (m_deleteButton)
    {
        m_deleteButton->setEnabled(hasAnnotationSelection);
    }
    if (m_applyButton)
    {
        m_applyButton->setEnabled(hasAnnotationSelection);
    }
    if (m_labelEdit)
    {
        m_labelEdit->setEnabled(hasAnnotationSelection);
    }
    if (m_bodyRegionComboBox)
    {
        m_bodyRegionComboBox->setEnabled(hasAnnotationSelection);
    }
    if (m_noteEdit)
    {
        m_noteEdit->setEnabled(hasAnnotationSelection);
    }
}

void VtkMprAnnotationDock::refreshEditor()
{
    if (!m_labelEdit || !m_bodyRegionComboBox || !m_noteEdit)
    {
        return;
    }

    const QString annotationId = selectedAnnotationId();
    const MprMeasurementAnnotationRecord record = m_recordsById.value(annotationId);
    const QSignalBlocker labelBlocker(m_labelEdit);
    const QSignalBlocker bodyRegionBlocker(m_bodyRegionComboBox);
    const QSignalBlocker noteBlocker(m_noteEdit);

    if (annotationId.isEmpty() || record.measurement.id.isEmpty())
    {
        m_labelEdit->clear();
        m_bodyRegionComboBox->setCurrentIndex(0);
        m_noteEdit->clear();
        return;
    }

    m_labelEdit->setText(record.label);
    int regionIndex = m_bodyRegionComboBox->findText(record.bodyRegion.trimmed().isEmpty()
        ? QStringLiteral("Other")
        : record.bodyRegion.trimmed());
    if (regionIndex < 0)
    {
        m_bodyRegionComboBox->addItem(record.bodyRegion.trimmed());
        regionIndex = m_bodyRegionComboBox->count() - 1;
    }
    m_bodyRegionComboBox->setCurrentIndex(regionIndex);
    m_noteEdit->setText(record.note);
}

QString VtkMprAnnotationDock::selectedAnnotationId() const
{
    if (!m_treeWidget)
    {
        return {};
    }

    QTreeWidgetItem* item = m_treeWidget->currentItem();
    if (!item)
    {
        return {};
    }
    return item->data(0, kAnnotationIdRole).toString();
}

QString VtkMprAnnotationDock::displayValue(const MprMeasurementAnnotationRecord& record)
{
    switch (record.measurement.type)
    {
    case MeasurementType::Distance:
    case MeasurementType::Polyline:
        return MeasurementService::formattedLength(record.measurement.lengthMm);
    case MeasurementType::Angle:
        return MeasurementService::formattedAngle(
            record.angleDegrees.value_or(MeasurementService::angleDegrees(record.measurement.points)));
    case MeasurementType::RectangleRoi:
        if (record.roiStatistics && record.roiStatistics->valid)
        {
            return QString("mean %1, n=%2")
                .arg(record.roiStatistics->mean, 0, 'f', 1)
                .arg(record.roiStatistics->sampleCount);
        }
        return "ROI";
    }

    return {};
}

QString VtkMprAnnotationDock::measurementTypeName(MeasurementType type)
{
    switch (type)
    {
    case MeasurementType::Distance:
        return "Distance";
    case MeasurementType::Polyline:
        return "Polyline";
    case MeasurementType::Angle:
        return "Angle";
    case MeasurementType::RectangleRoi:
        return "ROI";
    }

    return "Measurement";
}

QString VtkMprAnnotationDock::groupKey(const MprMeasurementAnnotationRecord& record)
{
    return QString("%1 | %2 mm")
        .arg(record.planeType.trimmed().isEmpty() ? QStringLiteral("MPR") : record.planeType.trimmed())
        .arg(record.planePositionMm, 0, 'f', 1);
}
