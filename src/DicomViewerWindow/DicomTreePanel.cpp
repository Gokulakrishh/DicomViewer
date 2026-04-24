#include "DicomTreePanel.h"

#include "DicomTreeItemRoles.h"
#include "Model/DicomParameters.h"

#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QSplitter>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTreeView>
#include <QVBoxLayout>

namespace TreeRoles = DicomTreeItemRoles;

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
    m_treeModel->setHorizontalHeaderLabels({"DICOM Database"});
    m_treeView->setModel(m_treeModel);
    m_treeView->setHeaderHidden(false);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setUniformRowHeights(true);

    m_previewTitleLabel = new QLabel("Preview", m_splitter);
    m_previewImageLabel = new QLabel(m_splitter);
    m_previewImageLabel->setAlignment(Qt::AlignCenter);
    m_previewImageLabel->setMinimumHeight(120);
    m_previewImageLabel->setMaximumHeight(180);
    m_previewImageLabel->setText("No preview");
    m_previewImageLabel->setFrameShape(QFrame::StyledPanel);
    m_previewImageLabel->setFrameShadow(QFrame::Sunken);

    auto* previewContainer = new QWidget(m_splitter);
    auto* previewLayout = new QVBoxLayout(previewContainer);
    previewLayout->setContentsMargins(4, 4, 4, 4);
    previewLayout->setSpacing(6);

    m_searchLineEdit = new QLineEdit(previewContainer);
    m_searchLineEdit->setPlaceholderText("Filter loaded tree...");
    m_globalSearchLineEdit = new QLineEdit(previewContainer);
    m_globalSearchLineEdit->setPlaceholderText("Global DB search by patient, doctor, modality, series...");

    previewLayout->addWidget(m_searchLineEdit);
    previewLayout->addWidget(m_globalSearchLineEdit);
    previewLayout->addWidget(m_previewTitleLabel);
    previewLayout->addWidget(m_previewImageLabel);
    previewLayout->addStretch();

    m_splitter->addWidget(m_treeView);
    m_splitter->addWidget(previewContainer);
    m_splitter->setStretchFactor(0, 3);
    m_splitter->setStretchFactor(1, 1);

    connect(m_treeView, &QTreeView::clicked, this, &DicomTreePanel::hierarchyItemActivated);
    connect(m_treeView, &QTreeView::expanded, this, &DicomTreePanel::hierarchyItemExpanded);
    connect(m_searchLineEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        applyTreeFilter(text);
        emit localSearchTextChanged(text);
    });
    connect(m_globalSearchLineEdit, &QLineEdit::textChanged, this, &DicomTreePanel::globalSearchTextChanged);
}

QStandardItem* DicomTreePanel::itemFromIndex(const QModelIndex& index) const
{
    return m_treeModel ? m_treeModel->itemFromIndex(index) : nullptr;
}

QString DicomTreePanel::localFilterText() const
{
    return m_searchLineEdit ? m_searchLineEdit->text() : QString();
}

QString DicomTreePanel::globalFilterText() const
{
    return m_globalSearchLineEdit ? m_globalSearchLineEdit->text().trimmed() : QString();
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
    const QList<std::shared_ptr<Series>>& seriesList)
{
    if (!studyItem || !patient || !study)
    {
        return;
    }

    for (const auto& series : seriesList)
    {
        addSeriesToTree(studyItem, patient, study, series);
    }
}

void DicomTreePanel::updatePreviewPane(const QPixmap& pixmap)
{
    if (!m_previewImageLabel)
    {
        return;
    }

    if (pixmap.isNull())
    {
        m_previewImageLabel->setPixmap(QPixmap());
        m_previewImageLabel->setText("No preview");
        return;
    }

    m_previewImageLabel->setText(QString());
    m_previewImageLabel->setPixmap(
        pixmap.scaled(m_previewImageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void DicomTreePanel::updateSeriesPreview(const std::shared_ptr<Series>& series)
{
    if (!series || series->previewPixmap().isNull())
    {
        updatePreviewPane(QPixmap());
        return;
    }

    updatePreviewPane(series->previewPixmap());
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
    patientItem->setData(TreeRoles::NodeTypePatient, TreeRoles::NodeType);
    patientItem->setData(false, TreeRoles::ChildrenLoaded);
    patientItem->setData(patient->patientId(), TreeRoles::PatientId);
    patientItem->setData(patient->patientName(), TreeRoles::PatientName);
    patientItem->setData(patient->dateOfBirth(), TreeRoles::PatientDob);
    patientItem->setData(
        QString("%1 %2 %3").arg(patient->patientId(), patient->patientName(), patient->dateOfBirth()),
        TreeRoles::SearchText);
    m_treeModel->invisibleRootItem()->appendRow(patientItem);
    patientItem->appendRow(new QStandardItem("Loading..."));
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
    studyItem->setData(TreeRoles::NodeTypeStudy, TreeRoles::NodeType);
    studyItem->setData(false, TreeRoles::ChildrenLoaded);
    studyItem->setData(patient->patientId(), TreeRoles::PatientId);
    studyItem->setData(study->studyInstanceUid(), TreeRoles::StudyInstanceUid);
    studyItem->setData(patient->patientName(), TreeRoles::PatientName);
    studyItem->setData(patient->dateOfBirth(), TreeRoles::PatientDob);
    studyItem->setData(study->doctorName(), TreeRoles::DoctorName);
    studyItem->setData(study->studyDate(), TreeRoles::StudyDate);
    studyItem->setData(
        QString("%1 %2 %3 %4 %5")
            .arg(patient->patientId(),
                 patient->patientName(),
                 patient->dateOfBirth(),
                 study->doctorName(),
                 study->studyDate()),
        TreeRoles::SearchText);
    studyItem->appendRow(new QStandardItem("Loading..."));
    patientItem->appendRow(studyItem);
}

void DicomTreePanel::addSeriesToTree(
    QStandardItem* studyItem,
    const std::shared_ptr<Patient>& patient,
    const std::shared_ptr<Study>& study,
    const std::shared_ptr<Series>& series)
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
    seriesItem->setData(TreeRoles::NodeTypeSeries, TreeRoles::NodeType);
    seriesItem->setData(true, TreeRoles::ChildrenLoaded);
    seriesItem->setData(series->seriesInstanceUid(), TreeRoles::SeriesInstanceUid);
    seriesItem->setData(patient->patientId(), TreeRoles::PatientId);
    seriesItem->setData(study->studyInstanceUid(), TreeRoles::StudyInstanceUid);
    seriesItem->setData(patient->patientName(), TreeRoles::PatientName);
    seriesItem->setData(patient->dateOfBirth(), TreeRoles::PatientDob);
    seriesItem->setData(study->doctorName(), TreeRoles::DoctorName);
    seriesItem->setData(series->modality(), TreeRoles::Modality);
    seriesItem->setData(study->studyDate(), TreeRoles::StudyDate);
    seriesItem->setData(
        QString("%1 %2 %3 %4 %5 %6")
            .arg(patient->patientId(),
                 patient->patientName(),
                 patient->dateOfBirth(),
                 study->doctorName(),
                 series->modality(),
                 study->studyDate()),
        TreeRoles::SearchText);

    if (series->imageCount() > 0)
    {
        seriesItem->setText(seriesLabel + QString(" | %1 slices").arg(series->imageCount()));
        seriesItem->setData(series->representativeFilePath(), TreeRoles::FilePath);
    }
    studyItem->appendRow(seriesItem);
}
