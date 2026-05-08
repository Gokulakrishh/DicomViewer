#include "DicomTreePanel.h"

#include "DicomTreeItemRoles.h"
#include "DicomPreviewPanel.h"
#include "Model/DicomParameters.h"

#include <QDate>
#include <QHeaderView>
#include <QLineEdit>
#include <QSplitter>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTreeView>
#include <QVBoxLayout>

namespace TreeRoles = DicomTreeItemRoles;

namespace
{
enum TreeColumn
{
    HierarchyColumn = 0,
    PatientIdColumn,
    AgeColumn,
    StudyDateColumn,
    ModalityColumn,
    SliceCountColumn,
    AnnotationColumn,
    ColumnCount
};

QStandardItem* makeReadOnlyItem(const QString& text = {})
{
    auto* item = new QStandardItem(text);
    item->setEditable(false);
    return item;
}

QDate parseDicomDate(const QString& value)
{
    const QString trimmedValue = value.trimmed();
    if (trimmedValue.isEmpty())
    {
        return {};
    }

    QDate date = QDate::fromString(trimmedValue, "yyyyMMdd");
    if (date.isValid())
    {
        return date;
    }

    date = QDate::fromString(trimmedValue, Qt::ISODate);
    return date.isValid() ? date : QDate();
}

QString ageAtStudyText(const QString& dateOfBirth, const QString& studyDate)
{
    const QDate birthDate = parseDicomDate(dateOfBirth);
    const QDate referenceDate = parseDicomDate(studyDate);
    if (!birthDate.isValid() || !referenceDate.isValid() || referenceDate < birthDate)
    {
        return {};
    }

    int years = referenceDate.year() - birthDate.year();
    if (birthDate.addYears(years) > referenceDate)
    {
        --years;
    }

    return years >= 0 ? QString("%1 y").arg(years) : QString();
}

QString annotationSummaryText(const AnnotationReportSummary& summary)
{
    if (!summary.hasAnnotations())
    {
        return {};
    }

    return QString("%1 / %2 slices").arg(summary.annotationCount).arg(summary.annotatedSliceCount);
}

QString annotationSummaryTooltip(const AnnotationReportSummary& summary)
{
    if (!summary.hasAnnotations())
    {
        return {};
    }

    QStringList parts;
    if (summary.distanceCount > 0)
    {
        parts.append(QString("Distance: %1").arg(summary.distanceCount));
    }
    if (summary.polylineCount > 0)
    {
        parts.append(QString("Polyline: %1").arg(summary.polylineCount));
    }
    if (summary.angleCount > 0)
    {
        parts.append(QString("Angle: %1").arg(summary.angleCount));
    }
    if (summary.rectangleRoiCount > 0)
    {
        parts.append(QString("ROI: %1").arg(summary.rectangleRoiCount));
    }

    return parts.join("\n");
}

void setStringRole(QStandardItem* item, int role, const QString& value)
{
    if (item)
    {
        item->setData(value, role);
    }
}

void setNodeType(QStandardItem* item, const char* nodeType)
{
    setStringRole(item, TreeRoles::NodeType, QString::fromUtf8(nodeType));
}

void setChildrenLoaded(QStandardItem* item, bool loaded)
{
    if (item)
    {
        item->setData(loaded, TreeRoles::ChildrenLoaded);
    }
}

}

DicomTreePanel::DicomTreePanel(QWidget* parent)
    : QWidget(parent)
{
    buildUi();
}

void DicomTreePanel::buildUi()
{
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    m_splitter = new QSplitter(Qt::Vertical, this);
    outerLayout->addWidget(m_splitter);

    m_treeView = new QTreeView(m_splitter);
    m_treeModel = new QStandardItemModel(this);
    m_treeModel->setColumnCount(ColumnCount);
    m_treeModel->setHorizontalHeaderLabels(
        {"DICOM Hierarchy", "Patient ID", "Age", "Study Date", "Modality", "Images", "Annotations"});
    m_treeView->setModel(m_treeModel);
    m_treeView->setHeaderHidden(false);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setRootIsDecorated(true);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_treeView->setTextElideMode(Qt::ElideRight);
    m_treeView->header()->setStretchLastSection(false);
    m_treeView->header()->setSectionsMovable(false);
    m_treeView->header()->setCascadingSectionResizes(false);
    m_treeView->header()->setMinimumSectionSize(48);
    m_treeView->header()->setSectionResizeMode(QHeaderView::Interactive);
    m_treeView->setColumnWidth(HierarchyColumn, 260);
    m_treeView->setColumnWidth(PatientIdColumn, 110);
    m_treeView->setColumnWidth(AgeColumn, 64);
    m_treeView->setColumnWidth(StudyDateColumn, 96);
    m_treeView->setColumnWidth(ModalityColumn, 72);
    m_treeView->setColumnWidth(SliceCountColumn, 68);
    m_treeView->setColumnWidth(AnnotationColumn, 104);

    auto* previewContainer = new QWidget(m_splitter);
    auto* previewLayout = new QVBoxLayout(previewContainer);
    previewLayout->setContentsMargins(4, 4, 4, 4);
    previewLayout->setSpacing(6);

    m_searchLineEdit = new QLineEdit(previewContainer);
    m_searchLineEdit->setPlaceholderText("Filter loaded tree...");
    m_globalSearchLineEdit = new QLineEdit(previewContainer);
    m_globalSearchLineEdit->setPlaceholderText("Global DB search by patient, doctor, modality, series...");
    m_previewPanel = new DicomPreviewPanel(previewContainer);

    previewLayout->addWidget(m_searchLineEdit);
    previewLayout->addWidget(m_globalSearchLineEdit);
    previewLayout->addWidget(m_previewPanel, 1);

    m_splitter->addWidget(m_treeView);
    m_splitter->addWidget(previewContainer);
    m_splitter->setStretchFactor(0, 3);
    m_splitter->setStretchFactor(1, 1);

    connect(m_treeView, &QTreeView::clicked, this, &DicomTreePanel::hierarchyItemActivated);
    connect(m_treeView, &QTreeView::expanded, this, &DicomTreePanel::hierarchyItemExpanded);
    connect(m_previewPanel, &DicomPreviewPanel::itemDoubleClicked, this, &DicomTreePanel::previewItemDoubleClicked);
    connect(m_searchLineEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        applyTreeFilter(text);
        emit localSearchTextChanged(text);
    });
    connect(m_globalSearchLineEdit, &QLineEdit::textChanged, this, &DicomTreePanel::globalSearchTextChanged);
}

QStandardItem* DicomTreePanel::itemFromIndex(const QModelIndex& index) const
{
    if (!m_treeModel || !index.isValid())
    {
        return nullptr;
    }

    return m_treeModel->itemFromIndex(index.sibling(index.row(), HierarchyColumn));
}

QString DicomTreePanel::localFilterText() const
{
    return m_searchLineEdit ? m_searchLineEdit->text() : QString();
}

QString DicomTreePanel::globalFilterText() const
{
    return m_globalSearchLineEdit ? m_globalSearchLineEdit->text().trimmed() : QString();
}

QStandardItem* DicomTreePanel::findPatientItem(const QString& patientId) const
{
    return findItemByRole(TreeRoles::PatientId, patientId, QString::fromUtf8(TreeRoles::NodeTypePatient));
}

QStandardItem* DicomTreePanel::findStudyItem(const QString& studyInstanceUid) const
{
    return findItemByRole(TreeRoles::StudyInstanceUid, studyInstanceUid, QString::fromUtf8(TreeRoles::NodeTypeStudy));
}

QStandardItem* DicomTreePanel::findSeriesItem(const QString& seriesInstanceUid) const
{
    return findItemByRole(TreeRoles::SeriesInstanceUid, seriesInstanceUid, QString::fromUtf8(TreeRoles::NodeTypeSeries));
}

void DicomTreePanel::selectAndRevealItem(QStandardItem* item)
{
    if (!item || !m_treeView)
    {
        return;
    }

    for (QStandardItem* parent = item->parent(); parent; parent = parent->parent())
    {
        m_treeView->expand(parent->index());
    }
    if (item->rowCount() > 0)
    {
        m_treeView->expand(item->index());
    }

    m_treeView->setCurrentIndex(item->index());
    m_treeView->scrollTo(item->index(), QAbstractItemView::PositionAtCenter);
}

void DicomTreePanel::refreshHierarchy(const QList<std::shared_ptr<Patient>>& patients)
{
    if (!m_treeModel)
    {
        return;
    }

    m_treeModel->removeRows(0, m_treeModel->rowCount());
    for (const auto& patient : patients)
    {
        addPatientToTree(patient);
    }

    applyTreeFilter(localFilterText());
}

void DicomTreePanel::appendStudies(
    QStandardItem* patientItem,
    const std::shared_ptr<Patient>& patient,
    const QList<std::shared_ptr<Study>>& studies)
{
    if (!patientItem || !patient)
    {
        return;
    }

    for (const auto& study : studies)
    {
        addStudyToTree(patientItem, patient, study);
    }
}

void DicomTreePanel::appendSeries(
    QStandardItem* studyItem,
    const std::shared_ptr<Patient>& patient,
    const std::shared_ptr<Study>& study,
    const QList<std::shared_ptr<Series>>& seriesList,
    const AnnotationReportSummaryBySeries& annotationSummaries)
{
    if (!studyItem || !patient || !study)
    {
        return;
    }

    for (const auto& series : seriesList)
    {
        addSeriesToTree(
            studyItem,
            patient,
            study,
            series,
            series ? annotationSummaries.value(series->seriesInstanceUid()) : AnnotationReportSummary{});
    }
}

void DicomTreePanel::updateSeriesAnnotationSummary(
    const QString& seriesInstanceUid,
    const AnnotationReportSummary& annotationSummary)
{
    if (!m_treeModel || seriesInstanceUid.trimmed().isEmpty())
    {
        return;
    }

    for (int row = 0; row < m_treeModel->rowCount(); ++row)
    {
        if (updateSeriesAnnotationSummaryRecursive(m_treeModel->item(row, HierarchyColumn), seriesInstanceUid, annotationSummary))
        {
            return;
        }
    }
}

void DicomTreePanel::updatePreviewPane(const QPixmap& pixmap)
{
    if (!m_previewPanel)
    {
        return;
    }

    m_previewPanel->showSinglePreview("Preview", pixmap);
}

void DicomTreePanel::updatePreviewItems(
    const QString& title,
    const DicomPreviewItems& items,
    const QString& emptyText)
{
    if (!m_previewPanel)
    {
        return;
    }

    m_previewPanel->showItems(title, items, emptyText);
}

void DicomTreePanel::applyTreeFilter(const QString& filterText)
{
    if (!m_treeModel || !m_treeView)
    {
        return;
    }

    const QString normalizedFilter = filterText.trimmed().toLower();
    for (int row = 0; row < m_treeModel->rowCount(); ++row)
    {
        QStandardItem* item = m_treeModel->item(row);
        const bool isVisible = updateItemVisibility(item, normalizedFilter);
        m_treeView->setRowHidden(row, QModelIndex(), !isVisible);
    }
}

bool DicomTreePanel::updateItemVisibility(QStandardItem* item, const QString& filterText)
{
    if (!item)
    {
        return false;
    }

    bool childVisible = false;
    for (int row = 0; row < item->rowCount(); ++row)
    {
        QStandardItem* childItem = item->child(row);
        const bool isChildVisible = updateItemVisibility(childItem, filterText);
        m_treeView->setRowHidden(row, item->index(), !isChildVisible);
        childVisible = childVisible || isChildVisible;
    }

    if (filterText.isEmpty())
    {
        return true;
    }

    const QString searchText = item->data(TreeRoles::SearchText).toString().toLower();
    return searchText.contains(filterText) || childVisible;
}

bool DicomTreePanel::updateSeriesAnnotationSummaryRecursive(
    QStandardItem* item,
    const QString& seriesInstanceUid,
    const AnnotationReportSummary& annotationSummary)
{
    if (!item)
    {
        return false;
    }

    if (item->data(TreeRoles::NodeType).toString() == TreeRoles::NodeTypeSeries &&
        item->data(TreeRoles::SeriesInstanceUid).toString() == seriesInstanceUid)
    {
        QStandardItem* parentItem = item->parent();
        QStandardItem* annotationItem = parentItem
            ? parentItem->child(item->row(), AnnotationColumn)
            : m_treeModel->item(item->row(), AnnotationColumn);
        if (annotationItem)
        {
            annotationItem->setText(annotationSummaryText(annotationSummary));
            annotationItem->setToolTip(annotationSummaryTooltip(annotationSummary));
        }
        return true;
    }

    for (int row = 0; row < item->rowCount(); ++row)
    {
        if (updateSeriesAnnotationSummaryRecursive(item->child(row, HierarchyColumn), seriesInstanceUid, annotationSummary))
        {
            return true;
        }
    }

    return false;
}

QStandardItem* DicomTreePanel::findItemByRole(
    int role,
    const QString& value,
    const QString& requiredNodeType) const
{
    if (!m_treeModel || value.trimmed().isEmpty())
    {
        return nullptr;
    }

    for (int row = 0; row < m_treeModel->rowCount(); ++row)
    {
        if (QStandardItem* item = findItemByRoleRecursive(
                m_treeModel->item(row, HierarchyColumn),
                role,
                value,
                requiredNodeType))
        {
            return item;
        }
    }

    return nullptr;
}

QStandardItem* DicomTreePanel::findItemByRoleRecursive(
    QStandardItem* item,
    int role,
    const QString& value,
    const QString& requiredNodeType) const
{
    if (!item)
    {
        return nullptr;
    }

    const bool nodeTypeMatches = requiredNodeType.trimmed().isEmpty() ||
        item->data(TreeRoles::NodeType).toString() == requiredNodeType;
    if (nodeTypeMatches && item->data(role).toString() == value)
    {
        return item;
    }

    for (int row = 0; row < item->rowCount(); ++row)
    {
        if (QStandardItem* foundItem = findItemByRoleRecursive(
                item->child(row, HierarchyColumn),
                role,
                value,
                requiredNodeType))
        {
            return foundItem;
        }
    }

    return nullptr;
}

QList<QStandardItem*> DicomTreePanel::makeRow(QStandardItem* primaryItem, const QStringList& columnValues) const
{
    QList<QStandardItem*> rowItems;
    rowItems.reserve(ColumnCount);
    primaryItem->setEditable(false);
    rowItems.append(primaryItem);

    for (int column = PatientIdColumn; column < ColumnCount; ++column)
    {
        const int valueIndex = column - PatientIdColumn;
        auto* item = makeReadOnlyItem(valueIndex < columnValues.size() ? columnValues.at(valueIndex) : QString());
        if (column == SliceCountColumn || column == AnnotationColumn)
        {
            item->setTextAlignment(Qt::AlignCenter);
        }
        rowItems.append(item);
    }

    return rowItems;
}

void DicomTreePanel::addPatientToTree(const std::shared_ptr<Patient>& patient)
{
    if (!patient || !m_treeModel)
    {
        return;
    }

    QString patientLabel = patient->patientName().trimmed();
    if (patientLabel.isEmpty())
    {
        patientLabel = patient->patientId().trimmed();
    }
    if (patientLabel.isEmpty())
    {
        patientLabel = "Unnamed Patient";
    }
    if (!patient->dateOfBirth().trimmed().isEmpty())
    {
        patientLabel += " | " + patient->dateOfBirth().trimmed();
    }

    auto* patientItem = new QStandardItem(patientLabel);
    setNodeType(patientItem, TreeRoles::NodeTypePatient);
    setChildrenLoaded(patientItem, false);
    setStringRole(patientItem, TreeRoles::PatientId, patient->patientId());
    setStringRole(patientItem, TreeRoles::PatientName, patient->patientName());
    setStringRole(patientItem, TreeRoles::PatientDob, patient->dateOfBirth());
    setStringRole(
        patientItem,
        TreeRoles::SearchText,
        QString("%1 %2 %3").arg(patient->patientId(), patient->patientName(), patient->dateOfBirth()));
    patientItem->setText(patientLabel);
    m_treeModel->invisibleRootItem()->appendRow(makeRow(patientItem, {patient->patientId(), {}, {}, {}, {}, {}}));
    patientItem->appendRow(makeRow(new QStandardItem("Loading..."), {}));
}

void DicomTreePanel::addStudyToTree(
    QStandardItem* patientItem,
    const std::shared_ptr<Patient>& patient,
    const std::shared_ptr<Study>& study)
{
    if (!patientItem || !patient || !study)
    {
        return;
    }

    QString studyLabel = study->studyDescription().trimmed();
    if (studyLabel.isEmpty())
    {
        studyLabel = "Unnamed Study";
    }
    if (!study->studyDate().trimmed().isEmpty())
    {
        studyLabel += " | " + study->studyDate().trimmed();
    }
    if (!study->doctorName().trimmed().isEmpty())
    {
        studyLabel += " | " + study->doctorName().trimmed();
    }

    auto* studyItem = new QStandardItem(studyLabel);
    setNodeType(studyItem, TreeRoles::NodeTypeStudy);
    setChildrenLoaded(studyItem, false);
    setStringRole(studyItem, TreeRoles::PatientId, patient->patientId());
    setStringRole(studyItem, TreeRoles::StudyInstanceUid, study->studyInstanceUid());
    setStringRole(studyItem, TreeRoles::PatientName, patient->patientName());
    setStringRole(studyItem, TreeRoles::PatientDob, patient->dateOfBirth());
    setStringRole(studyItem, TreeRoles::DoctorName, study->doctorName());
    setStringRole(studyItem, TreeRoles::StudyDate, study->studyDate());
    setStringRole(
        studyItem,
        TreeRoles::SearchText,
        QString("%1 %2 %3 %4 %5")
            .arg(patient->patientId(),
                 patient->patientName(),
                 patient->dateOfBirth(),
                 study->doctorName(),
                 study->studyDate()));
    studyItem->setText(studyLabel);
    patientItem->appendRow(
        makeRow(
            studyItem,
            {patient->patientId(), ageAtStudyText(patient->dateOfBirth(), study->studyDate()), study->studyDate(), {}, {}, {}}));
    studyItem->appendRow(makeRow(new QStandardItem("Loading..."), {}));
}

void DicomTreePanel::addSeriesToTree(
    QStandardItem* studyItem,
    const std::shared_ptr<Patient>& patient,
    const std::shared_ptr<Study>& study,
    const std::shared_ptr<Series>& series,
    const AnnotationReportSummary& annotationSummary)
{
    if (!studyItem || !patient || !study || !series)
    {
        return;
    }

    QString seriesLabel = series->modality().trimmed();
    const QString seriesDescription = series->seriesDescription().trimmed();
    if (!seriesDescription.isEmpty())
    {
        seriesLabel = seriesLabel.isEmpty() ? seriesDescription : seriesLabel + " | " + seriesDescription;
    }
    if (seriesLabel.isEmpty())
    {
        seriesLabel = "Unnamed Series";
    }

    auto* seriesItem = new QStandardItem(seriesLabel);
    setNodeType(seriesItem, TreeRoles::NodeTypeSeries);
    setChildrenLoaded(seriesItem, true);
    setStringRole(seriesItem, TreeRoles::SeriesInstanceUid, series->seriesInstanceUid());
    setStringRole(seriesItem, TreeRoles::PatientId, patient->patientId());
    setStringRole(seriesItem, TreeRoles::StudyInstanceUid, study->studyInstanceUid());
    setStringRole(seriesItem, TreeRoles::PatientName, patient->patientName());
    setStringRole(seriesItem, TreeRoles::PatientDob, patient->dateOfBirth());
    setStringRole(seriesItem, TreeRoles::DoctorName, study->doctorName());
    setStringRole(seriesItem, TreeRoles::Modality, series->modality());
    setStringRole(seriesItem, TreeRoles::StudyDate, study->studyDate());
    setStringRole(
        seriesItem,
        TreeRoles::SearchText,
        QString("%1 %2 %3 %4 %5 %6 %7")
            .arg(patient->patientId(),
                 patient->patientName(),
                 patient->dateOfBirth(),
                 study->doctorName(),
                 series->modality(),
                 study->studyDate(),
                 annotationSummaryTooltip(annotationSummary)));
    seriesItem->setText(seriesLabel);

    if (series->imageCount() > 0)
    {
        setStringRole(seriesItem, TreeRoles::FilePath, series->representativeFilePath());
    }

    const QString annotationText = annotationSummaryText(annotationSummary);
    QList<QStandardItem*> row = makeRow(
        seriesItem,
        {patient->patientId(),
         ageAtStudyText(patient->dateOfBirth(), study->studyDate()),
         study->studyDate(),
         series->modality(),
         series->imageCount() > 0 ? QString::number(series->imageCount()) : QString(),
         annotationText});
    if (!annotationText.isEmpty())
    {
        row.at(AnnotationColumn)->setToolTip(annotationSummaryTooltip(annotationSummary));
    }
    studyItem->appendRow(row);
}
