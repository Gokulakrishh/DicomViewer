#include "DicomMainWindow.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QDate>
#include <QDebug>
#include <QDockWidget>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QFutureWatcher>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QSet>
#include <QSlider>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStatusBar>
#include <QStringList>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent/QtConcurrent>
#include <algorithm>
#include <cmath>

#include "Config/QSettingsDatabaseConfigService.h"
#include "Database/PostgreService.h"
#include "FileHandling/GDCMFileHandling.h"
#include "Model/MedicalImage.h"
#include "UI/WarningDialogService.h"

constexpr int kFilePathRole = Qt::UserRole + 1;
constexpr int kSeriesInstanceUidRole = Qt::UserRole + 2;
constexpr int kPatientNameRole = Qt::UserRole + 3;
constexpr int kPatientDobRole = Qt::UserRole + 4;
constexpr int kDoctorNameRole = Qt::UserRole + 5;
constexpr int kModalityRole = Qt::UserRole + 6;
constexpr int kStudyDateRole = Qt::UserRole + 7;
constexpr int kSearchTextRole = Qt::UserRole + 8;

DicomMainWindow::DicomMainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_ui(new Ui::DicomMainWindow)
{
    m_ui->setupUi(this);
    setUiComponents();
    resize(1200, 800);
}

DicomMainWindow::~DicomMainWindow()
{
    delete m_ui;
}

void DicomMainWindow::setUiComponents()
{
    setupMenuBar();
    setupImageControlsDock();

    m_leftPanelSplitter = new QSplitter(Qt::Vertical, this);

    m_treeView = new QTreeView(m_leftPanelSplitter);
    m_treeModel = new QStandardItemModel(this);
    m_treeModel->setHorizontalHeaderLabels({"DICOM Database"});
    m_treeView->setModel(m_treeModel);
    m_treeView->setHeaderHidden(false);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setUniformRowHeights(true);
    m_previewTitleLabel = new QLabel("Preview", m_leftPanelSplitter);
    m_previewImageLabel = new QLabel(m_leftPanelSplitter);
    m_previewImageLabel->setAlignment(Qt::AlignCenter);
    m_previewImageLabel->setMinimumHeight(120);
    m_previewImageLabel->setMaximumHeight(180);
    m_previewImageLabel->setText("No preview");
    m_previewImageLabel->setFrameShape(QFrame::StyledPanel);
    m_previewImageLabel->setFrameShadow(QFrame::Sunken);

    auto* previewContainer = new QWidget(m_leftPanelSplitter);
    auto* previewLayout = new QVBoxLayout(previewContainer);
    previewLayout->setContentsMargins(4, 4, 4, 4);
    previewLayout->setSpacing(6);
    m_searchLineEdit = new QLineEdit(previewContainer);
    m_searchLineEdit->setPlaceholderText("Search by patient, DOB, doctor, patient ID...");
    previewLayout->addWidget(m_searchLineEdit);
    previewLayout->addWidget(m_previewTitleLabel);
    previewLayout->addWidget(m_previewImageLabel);

    auto* detailsLayout = new QGridLayout();
    detailsLayout->setHorizontalSpacing(10);
    detailsLayout->setVerticalSpacing(4);
    m_patientNameValueLabel = new QLabel("-", previewContainer);
    m_patientDobValueLabel = new QLabel("-", previewContainer);
    m_patientAgeValueLabel = new QLabel("-", previewContainer);
    m_doctorValueLabel = new QLabel("-", previewContainer);
    m_modalityValueLabel = new QLabel("-", previewContainer);
    m_studyDateValueLabel = new QLabel("-", previewContainer);

    for (QLabel* label :
         {m_patientNameValueLabel,
          m_patientDobValueLabel,
          m_patientAgeValueLabel,
          m_doctorValueLabel,
          m_modalityValueLabel,
          m_studyDateValueLabel})
    {
        label->setWordWrap(true);
    }

    detailsLayout->addWidget(new QLabel("Name", previewContainer), 0, 0);
    detailsLayout->addWidget(m_patientNameValueLabel, 0, 1);
    detailsLayout->addWidget(new QLabel("DOB", previewContainer), 0, 2);
    detailsLayout->addWidget(m_patientDobValueLabel, 0, 3);
    detailsLayout->addWidget(new QLabel("Age", previewContainer), 1, 0);
    detailsLayout->addWidget(m_patientAgeValueLabel, 1, 1);
    detailsLayout->addWidget(new QLabel("Doctor", previewContainer), 1, 2);
    detailsLayout->addWidget(m_doctorValueLabel, 1, 3);
    detailsLayout->addWidget(new QLabel("Modality", previewContainer), 2, 0);
    detailsLayout->addWidget(m_modalityValueLabel, 2, 1);
    detailsLayout->addWidget(new QLabel("Scan Date", previewContainer), 2, 2);
    detailsLayout->addWidget(m_studyDateValueLabel, 2, 3);
    detailsLayout->setColumnStretch(1, 1);
    detailsLayout->setColumnStretch(3, 1);
    previewLayout->addLayout(detailsLayout);
    previewLayout->addStretch();

    m_leftPanelSplitter->addWidget(m_treeView);
    m_leftPanelSplitter->addWidget(previewContainer);
    m_leftPanelSplitter->setStretchFactor(0, 3);
    m_leftPanelSplitter->setStretchFactor(1, 1);

    m_ui->horizontalLayout->replaceWidget(m_ui->listFileView, m_leftPanelSplitter);
    m_ui->listFileView->deleteLater();

    m_view = new DicomGraphicsView(this);
    m_ui->horizontalLayout->replaceWidget(m_ui->graphicsView, m_view);
    m_ui->graphicsView->deleteLater();
    m_ui->horizontalLayout->setSpacing(8);

    if (m_ui->viewerControlsSeparator)
    {
        m_ui->viewerControlsSeparator->setStyleSheet("color: palette(mid);");
    }

    if (m_ui->imageVerticalSlider)
    {
        m_ui->imageVerticalSlider->setStyleSheet("QSlider { background: transparent; }");
    }

    if (m_ui->cineCheckBox)
    {
        m_ui->cineCheckBox->setStyleSheet("QCheckBox::indicator { subcontrol-position: center; }");
    }

    m_ui->contrastVerticalSlider->hide();
    m_ui->imageVerticalSlider->setEnabled(false);
    m_ui->imageVerticalSlider->setMinimum(0);
    m_ui->imageVerticalSlider->setMaximum(0);
    m_ui->imageVerticalSlider->setValue(0);
    m_ui->cineCheckBox->setEnabled(false);
    m_ui->cineCheckBox->setChecked(false);

    m_cineTimer = new QTimer(this);
    m_cineTimer->setInterval(100);

    m_gdcmHandler = std::make_unique<GDCMFileHandling>();
    m_viewportController = std::make_unique<DicomViewportController>(m_gdcmHandler.get(), &m_renderService, this);
    m_warningDialogService = std::make_unique<WarningDialogService>(this);
    QSettingsDatabaseConfigService databaseConfigService;
    m_databaseService = std::make_unique<PostgreService>(databaseConfigService.loadDatabaseSettings());

    if (!m_databaseService->initialize())
    {
        const QString warningMessage =
            m_databaseService->lastErrorText() + " Config file: " + databaseConfigService.configFilePath();
        statusBar()->showMessage(warningMessage, 10000);
        m_warningDialogService->showWarning("Database Configuration", warningMessage);
    }

    setupConnections();
    refreshHierarchyTree();
}

void DicomMainWindow::setupMenuBar()
{
    m_openFileAction = new QAction("Open File", this);
    m_ui->fileMenu->addAction(m_openFileAction);

    m_openFolderAction = new QAction("Open Folder", this);
    m_ui->fileMenu->addAction(m_openFolderAction);

    connect(m_openFileAction, &QAction::triggered, this, &DicomMainWindow::openImage);
    connect(m_openFolderAction, &QAction::triggered, this, &DicomMainWindow::openFolder);
}

void DicomMainWindow::setupConnections()
{
    if (m_treeView)
    {
        connect(m_treeView, &QTreeView::clicked, this, &DicomMainWindow::onHierarchyItemActivated);
    }

    if (m_ui->imageVerticalSlider)
    {
        connect(m_ui->imageVerticalSlider, &QSlider::valueChanged, this, &DicomMainWindow::onImageSliderValueChanged);
    }

    if (m_ui->cineCheckBox)
    {
        connect(m_ui->cineCheckBox, &QCheckBox::toggled, this, &DicomMainWindow::onCineToggled);
    }

    if (m_searchLineEdit)
    {
        connect(m_searchLineEdit, &QLineEdit::textChanged, this, &DicomMainWindow::onSearchTextChanged);
    }

    if (m_view)
    {
        connect(m_view, &DicomGraphicsView::wheelSliceNavigationRequested, this, &DicomMainWindow::onSliceWheelRequested);
        connect(m_view, &DicomGraphicsView::distanceMeasurementRequested, this, &DicomMainWindow::onDistanceMeasurementRequested);
        connect(m_view, &DicomGraphicsView::pixelProbeRequested, this, &DicomMainWindow::onPixelProbeRequested);
        connect(m_view, &DicomGraphicsView::angleMeasurementRequested, this, &DicomMainWindow::onAngleMeasurementRequested);
    }

    if (m_cineTimer)
    {
        connect(m_cineTimer, &QTimer::timeout, this, &DicomMainWindow::advanceCinePlayback);
    }

    if (m_windowLevelSlider)
    {
        connect(m_windowLevelSlider, &QSlider::valueChanged, this, &DicomMainWindow::onWindowLevelChanged);
    }

    if (m_windowWidthSlider)
    {
        connect(m_windowWidthSlider, &QSlider::valueChanged, this, &DicomMainWindow::onWindowWidthChanged);
    }

    if (m_presetComboBox)
    {
        connect(m_presetComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DicomMainWindow::onPresetChanged);
    }

    if (m_toolComboBox)
    {
        connect(m_toolComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DicomMainWindow::onToolChanged);
    }
}

void DicomMainWindow::refreshHierarchyTree()
{
    if (!m_treeModel)
    {
        return;
    }

    m_treeModel->removeRows(0, m_treeModel->rowCount());
    if (!m_databaseService)
    {
        return;
    }

    const QList<DatabaseService::PatientPtr> patients = m_databaseService->getAllPatients();
    for (const auto& patient : patients)
    {
        addPatientToTree(patient);
    }

    m_treeView->expandToDepth(2);
    applyTreeFilter(m_searchLineEdit ? m_searchLineEdit->text() : QString());
}

void DicomMainWindow::addPatientToTree(const std::shared_ptr<Patient>& patient)
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
    patientItem->setData(patient->patientName(), kPatientNameRole);
    patientItem->setData(patient->dateOfBirth(), kPatientDobRole);
    patientItem->setData(
        QString("%1 %2 %3").arg(patient->patientId(), patient->patientName(), patient->dateOfBirth()),
        kSearchTextRole);
    m_treeModel->invisibleRootItem()->appendRow(patientItem);

    for (const auto& [studyInstanceUid, study] : patient->studyMap())
    {
        Q_UNUSED(studyInstanceUid);
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
        studyItem->setData(patient->patientName(), kPatientNameRole);
        studyItem->setData(patient->dateOfBirth(), kPatientDobRole);
        studyItem->setData(study->doctorName(), kDoctorNameRole);
        studyItem->setData(study->studyDate(), kStudyDateRole);
        studyItem->setData(
            QString("%1 %2 %3 %4 %5")
                .arg(patient->patientId(),
                     patient->patientName(),
                     patient->dateOfBirth(),
                     study->doctorName(),
                     study->studyDate()),
            kSearchTextRole);
        patientItem->appendRow(studyItem);

        for (const auto& [seriesInstanceUid, series] : study->seriesMap())
        {
            Q_UNUSED(seriesInstanceUid);
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
            seriesItem->setData(series->seriesInstanceUid(), kSeriesInstanceUidRole);
            seriesItem->setData(patient->patientName(), kPatientNameRole);
            seriesItem->setData(patient->dateOfBirth(), kPatientDobRole);
            seriesItem->setData(study->doctorName(), kDoctorNameRole);
            seriesItem->setData(series->modality(), kModalityRole);
            seriesItem->setData(study->studyDate(), kStudyDateRole);
            seriesItem->setData(
                QString("%1 %2 %3 %4 %5 %6")
                    .arg(patient->patientId(),
                         patient->patientName(),
                         patient->dateOfBirth(),
                         study->doctorName(),
                         series->modality(),
                         study->studyDate()),
                kSearchTextRole);

            const auto& images = series->images();
            if (!images.empty() && images.front())
            {
                seriesItem->setText(seriesLabel + QString(" | %1 slices").arg(images.size()));
                seriesItem->setData(images.front()->filePath(), kFilePathRole);
            }

            studyItem->appendRow(seriesItem);
        }
    }
}

void DicomMainWindow::clearCurrentSeries()
{
    if (m_viewportController)
    {
        m_viewportController->clear();
    }
    if (m_view)
    {
        m_view->clearMeasurementOverlays();
    }

    if (m_cineTimer)
    {
        m_cineTimer->stop();
    }

    if (m_ui->imageVerticalSlider)
    {
        m_ui->imageVerticalSlider->blockSignals(true);
        m_ui->imageVerticalSlider->setEnabled(false);
        m_ui->imageVerticalSlider->setMinimum(0);
        m_ui->imageVerticalSlider->setMaximum(0);
        m_ui->imageVerticalSlider->setValue(0);
        m_ui->imageVerticalSlider->blockSignals(false);
    }

    updateCineControls();
    updateImageControlsState(true);
    updatePreviewPane(QPixmap());
}

void DicomMainWindow::updatePatientInfoPanel(QStandardItem* item)
{
    const QString patientName = item ? item->data(kPatientNameRole).toString() : QString();
    const QString patientDob = item ? item->data(kPatientDobRole).toString() : QString();
    const QString doctorName = item ? item->data(kDoctorNameRole).toString() : QString();
    const QString modality = item ? item->data(kModalityRole).toString() : QString();
    const QString studyDate = item ? item->data(kStudyDateRole).toString() : QString();

    QDate dobDate = QDate::fromString(patientDob, "yyyy-MM-dd");
    if (!dobDate.isValid())
    {
        dobDate = QDate::fromString(patientDob, "yyyyMMdd");
    }

    QString ageText = "-";
    if (dobDate.isValid())
    {
        ageText = QString::number(dobDate.daysTo(QDate::currentDate()) / 365);
    }

    m_patientNameValueLabel->setText(patientName.isEmpty() ? "-" : patientName);
    m_patientDobValueLabel->setText(patientDob.isEmpty() ? "-" : patientDob);
    m_patientAgeValueLabel->setText(ageText);
    m_doctorValueLabel->setText(doctorName.isEmpty() ? "-" : doctorName);
    m_modalityValueLabel->setText(modality.isEmpty() ? "-" : modality);
    m_studyDateValueLabel->setText(studyDate.isEmpty() ? "-" : studyDate);
}

void DicomMainWindow::updatePreviewPane(const QPixmap& pixmap)
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

void DicomMainWindow::updateSeriesPreview(const std::shared_ptr<Series>& series)
{
    if (!series || series->images().empty() || !series->images().front())
    {
        updatePreviewPane(QPixmap());
        return;
    }

    auto& previewImage = *series->images().front();
    if (m_viewportController && m_viewportController->ensureImageLoaded(previewImage))
    {
        updatePreviewPane(previewImage.pixmap());
    }
    else
    {
        updatePreviewPane(QPixmap());
    }
}

void DicomMainWindow::applyTreeFilter(const QString& filterText)
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

bool DicomMainWindow::updateItemVisibility(QStandardItem* item, const QString& filterText)
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

    const QString searchText = item->data(kSearchTextRole).toString().toLower();
    return searchText.contains(filterText) || childVisible;
}

void DicomMainWindow::updateCineControls()
{
    const bool hasPlayableSeries = m_viewportController && m_viewportController->hasPlayableSeries();

    if (m_ui->cineCheckBox)
    {
        m_ui->cineCheckBox->blockSignals(true);
        m_ui->cineCheckBox->setEnabled(hasPlayableSeries);
        if (!hasPlayableSeries)
        {
            m_ui->cineCheckBox->setChecked(false);
            if (m_viewportController)
            {
                m_viewportController->setCinePlaying(false);
            }
        }
        m_ui->cineCheckBox->blockSignals(false);
    }
}

void DicomMainWindow::displayCurrentSlice()
{
    if (!m_viewportController || !m_viewportController->currentSeries())
    {
        m_view->clearImage();
        return;
    }

    QString failedFilePath;
    const bool cinePlaying = m_ui->cineCheckBox && m_ui->cineCheckBox->isChecked();
    if (!m_viewportController->prepareCurrentSeriesImage(cinePlaying, &failedFilePath))
    {
        const QString warningMessage = "Failed to load image: " + QFileInfo(failedFilePath).fileName();
        statusBar()->showMessage(warningMessage, 4000);
        m_warningDialogService->showWarning("Image Loading", warningMessage);
        m_view->clearImage();
        return;
    }

    updateImageControlsState(false);
    applyImageAdjustments();

    const auto currentSeries = m_viewportController->currentSeries();
    statusBar()->showMessage(
        QString("Loaded slice %1/%2: %3")
            .arg(m_viewportController->currentImageIndex() + 1)
            .arg(currentSeries ? currentSeries->images().size() : 0)
            .arg(QFileInfo(failedFilePath.isEmpty()
                               ? currentSeries->images()[static_cast<std::size_t>(m_viewportController->currentImageIndex())]->filePath()
                               : failedFilePath)
                     .fileName()),
        4000);
}

void DicomMainWindow::loadSeries(const std::shared_ptr<Series>& series, int initialIndex)
{
    if (!series || series->images().empty())
    {
        clearCurrentSeries();
        m_view->clearImage();
        return;
    }

    m_viewportController->setSeries(series, initialIndex);
    updateSeriesPreview(series);
    const int imageCount = m_viewportController->imageCount();

    if (m_ui->imageVerticalSlider)
    {
        m_ui->imageVerticalSlider->blockSignals(true);
        m_ui->imageVerticalSlider->setEnabled(imageCount > 1);
        m_ui->imageVerticalSlider->setMinimum(0);
        m_ui->imageVerticalSlider->setMaximum(imageCount - 1);
        m_ui->imageVerticalSlider->setValue(m_viewportController->currentImageIndex());
        m_ui->imageVerticalSlider->blockSignals(false);
    }

    updateCineControls();
    displayCurrentSlice();
}

void DicomMainWindow::loadAndDisplayImage(const QString& filePath)
{
    if (!m_gdcmHandler)
    {
        statusBar()->showMessage("File handler is not available.", 4000);
        m_warningDialogService->showWarning("File Loading", "File handler is not available.");
        return;
    }

    std::unique_ptr<MedicalImage> loadedImage = m_gdcmHandler->loadImage(filePath);
    if (!loadedImage || !loadedImage->isValid())
    {
        const QString warningMessage = "Failed to load image: " + QFileInfo(filePath).fileName();
        statusBar()->showMessage(warningMessage, 4000);
        m_warningDialogService->showWarning("Image Loading", warningMessage);
        m_view->clearImage();
        return;
    }

    if (auto* dicomImage = dynamic_cast<DicomImage*>(loadedImage.get()))
    {
        auto singleImage = std::shared_ptr<DicomImage>(static_cast<DicomImage*>(loadedImage.release()));
        m_viewportController->setSingleImage(singleImage);
        updateImageControlsState(true);
        updatePreviewPane(singleImage->pixmap());
        applyImageAdjustments();
        return;
    }

    std::shared_ptr<MedicalImage> image = std::move(loadedImage);
    m_view->setImage(std::move(image));
    statusBar()->showMessage("Loaded: " + QFileInfo(filePath).fileName(), 4000);
}

void DicomMainWindow::openImage()
{
    QStringList supportedFormats;
    if (m_gdcmHandler)
    {
        supportedFormats = m_gdcmHandler->getSupportedFormats();
    }

    QString filter = "DICOM/Image Files (*.dcm *.png *.jpg *.jpeg *.bmp)";
    if (!supportedFormats.isEmpty())
    {
        filter = "Supported Files (" + supportedFormats.join(' ') + ")";
    }

    const QString fileName = QFileDialog::getOpenFileName(
        this,
        "Open Image",
        QString(),
        filter + ";;All Files (*)");

    if (fileName.isEmpty())
    {
        return;
    }

    clearCurrentSeries();
    loadAndDisplayImage(fileName);

    QString statusMessage = "Loaded: " + QFileInfo(fileName).fileName();
    if (m_databaseService && m_gdcmHandler)
    {
        FileHandling::PatientPtr patientHierarchy = m_gdcmHandler->loadDicomHierarchy(fileName);
        if (patientHierarchy)
        {
            if (m_databaseService->savePatient(patientHierarchy))
            {
                statusMessage += " | Imported into PostgreSQL";
                refreshHierarchyTree();
            }
            else
            {
                statusMessage += " | PostgreSQL save failed";
                m_warningDialogService->showWarning("Database Import", "Failed to save the selected DICOM into PostgreSQL.");
            }
        }
    }

    statusBar()->showMessage(statusMessage, 5000);
}

void DicomMainWindow::openFolder()
{
    const QString folderPath = QFileDialog::getExistingDirectory(this, "Open DICOM Folder");
    if (folderPath.isEmpty())
    {
        return;
    }

    if (!m_gdcmHandler || !m_databaseService)
    {
        statusBar()->showMessage("Folder import is not available.", 4000);
        m_warningDialogService->showWarning("Folder Import", "Folder import is not available.");
        return;
    }

    const FileHandling::PatientList patients = m_gdcmHandler->loadDicomFolder(folderPath);
    if (patients.isEmpty())
    {
        statusBar()->showMessage("No importable DICOM files found in folder.", 5000);
        m_warningDialogService->showWarning("Folder Import", "No importable DICOM files found in the selected folder.");
        return;
    }

    int importedPatientCount = 0;
    for (const auto& patient : patients)
    {
        if (m_databaseService->savePatient(patient))
        {
            ++importedPatientCount;
        }
    }

    if (importedPatientCount == 0)
    {
        m_warningDialogService->showWarning("Database Import", "The folder was scanned, but no patient hierarchy could be saved into PostgreSQL.");
    }

    refreshHierarchyTree();
    m_treeView->expandAll();
    statusBar()->showMessage(
        QString("Imported folder %1 | Patients saved: %2").arg(QFileInfo(folderPath).fileName()).arg(importedPatientCount),
        6000);
}

void DicomMainWindow::onHierarchyItemActivated(const QModelIndex& index)
{
    if (!index.isValid() || !m_treeModel)
    {
        return;
    }

    QStandardItem* item = m_treeModel->itemFromIndex(index);
    if (!item)
    {
        return;
    }

    updatePatientInfoPanel(item);

    const QString seriesInstanceUid = item->data(kSeriesInstanceUidRole).toString();
    if (!seriesInstanceUid.isEmpty() && m_databaseService)
    {
        loadSeries(m_databaseService->getSeries(seriesInstanceUid));
        return;
    }

    const QString filePath = item->data(kFilePathRole).toString();
    if (filePath.isEmpty())
    {
        return;
    }

    clearCurrentSeries();
    loadAndDisplayImage(filePath);
}

void DicomMainWindow::onImageSliderValueChanged(int value)
{
    if (!m_viewportController || !m_viewportController->currentSeries() || value == m_viewportController->currentImageIndex())
    {
        return;
    }

    m_viewportController->setCurrentImageIndex(value);
    displayCurrentSlice();
}

void DicomMainWindow::onSliceWheelRequested(int stepCount)
{
    if (!m_viewportController || !m_viewportController->currentSeries() || stepCount == 0)
    {
        return;
    }

    const int nextIndex = m_viewportController->clampedIndexWithStep(stepCount);
    if (nextIndex == m_viewportController->currentImageIndex())
    {
        return;
    }

    if (m_ui->imageVerticalSlider)
    {
        m_ui->imageVerticalSlider->setValue(nextIndex);
    }
}

void DicomMainWindow::onCineToggled(bool checked)
{
    if (!m_cineTimer)
    {
        return;
    }

    if (m_viewportController)
    {
        m_viewportController->setCinePlaying(checked);
    }

    if (checked && m_viewportController && m_viewportController->hasPlayableSeries())
    {
        m_cineTimer->start();
    }
    else
    {
        m_cineTimer->stop();
    }
}

void DicomMainWindow::advanceCinePlayback()
{
    if (!m_viewportController || !m_viewportController->hasPlayableSeries() || !m_ui->imageVerticalSlider)
    {
        return;
    }

    m_ui->imageVerticalSlider->setValue(m_viewportController->nextCineIndex());
}

void DicomMainWindow::onSearchTextChanged(const QString& text)
{
    applyTreeFilter(text);
}

void DicomMainWindow::setupImageControlsDock()
{
    m_imageControlsDock = new QDockWidget("Image Controls", this);
    m_imageControlsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_imageControlsDock->setFeatures(
        QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);

    auto* dockContentWidget = new QWidget(m_imageControlsDock);
    auto* dockLayout = new QVBoxLayout(dockContentWidget);

    m_presetComboBox = new QComboBox(dockContentWidget);
    m_presetComboBox->addItem("Custom");
    m_presetComboBox->addItem("CT Bone");
    m_presetComboBox->addItem("CT Lung");
    m_presetComboBox->addItem("CT Brain");
    m_presetComboBox->addItem("Soft Tissue");
    dockLayout->addWidget(new QLabel("Preset", dockContentWidget));
    dockLayout->addWidget(m_presetComboBox);

    m_toolComboBox = new QComboBox(dockContentWidget);
    m_toolComboBox->addItem("Pan");
    m_toolComboBox->addItem("Distance");
    m_toolComboBox->addItem("Pixel Probe");
    m_toolComboBox->addItem("Angle");
    dockLayout->addWidget(new QLabel("Tool", dockContentWidget));
    dockLayout->addWidget(m_toolComboBox);

    m_windowLevelValueLabel = new QLabel("0", dockContentWidget);
    m_windowLevelSlider = new QSlider(Qt::Horizontal, dockContentWidget);
    m_windowLevelSlider->setRange(-100, 100);
    m_windowLevelSlider->setValue(0);
    dockLayout->addWidget(new QLabel("Window Level (Brightness)", dockContentWidget));
    dockLayout->addWidget(m_windowLevelValueLabel);
    dockLayout->addWidget(m_windowLevelSlider);

    m_windowWidthValueLabel = new QLabel("100", dockContentWidget);
    m_windowWidthSlider = new QSlider(Qt::Horizontal, dockContentWidget);
    m_windowWidthSlider->setRange(10, 300);
    m_windowWidthSlider->setValue(100);
    dockLayout->addWidget(new QLabel("Window Width (Contrast)", dockContentWidget));
    dockLayout->addWidget(m_windowWidthValueLabel);
    dockLayout->addWidget(m_windowWidthSlider);
    dockLayout->addStretch();

    m_imageControlsDock->setWidget(dockContentWidget);
    addDockWidget(Qt::RightDockWidgetArea, m_imageControlsDock);
}

void DicomMainWindow::updateImageControlsState(bool resetWindowState)
{
    if (!m_viewportController)
    {
        return;
    }

    const auto state = m_viewportController->windowControlState(resetWindowState);
    const bool hasImage = state.hasImage;

    if (m_imageControlsDock)
    {
        m_imageControlsDock->setEnabled(hasImage);
    }

    if (!hasImage || !m_windowLevelSlider || !m_windowWidthSlider)
    {
        return;
    }

    m_windowLevelSlider->blockSignals(true);
    m_windowWidthSlider->blockSignals(true);

    m_windowLevelSlider->setRange(state.levelMin, state.levelMax);
    m_windowWidthSlider->setRange(state.widthMin, state.widthMax);
    m_windowLevelSlider->setValue(state.level);
    m_windowWidthSlider->setValue(state.width);
    m_windowLevelSlider->blockSignals(false);
    m_windowWidthSlider->blockSignals(false);

    if (m_windowLevelValueLabel)
    {
        m_windowLevelValueLabel->setText(QString::number(state.level));
    }
    if (m_windowWidthValueLabel)
    {
        m_windowWidthValueLabel->setText(QString::number(state.width));
    }
    if (m_presetComboBox)
    {
        m_presetComboBox->blockSignals(true);
        m_presetComboBox->setCurrentIndex(state.presetIndex);
        m_presetComboBox->blockSignals(false);
    }
}

void DicomMainWindow::applyImageAdjustments()
{
    if (!m_viewportController)
    {
        m_view->clearImage();
        return;
    }

    const int windowLevel = m_viewportController->currentWindowLevel();
    const int windowWidth = m_viewportController->currentWindowWidth();

    if (m_windowLevelValueLabel)
    {
        m_windowLevelValueLabel->setText(QString::number(windowLevel));
    }
    if (m_windowWidthValueLabel)
    {
        m_windowWidthValueLabel->setText(QString::number(windowWidth));
    }

    auto adjustedImageModel = m_viewportController->renderCurrentImage();
    if (!adjustedImageModel)
    {
        m_view->clearImage();
        return;
    }
    m_view->setImage(adjustedImageModel);
}

void DicomMainWindow::onToolChanged(int index)
{
    if (!m_view)
    {
        return;
    }

    if (m_viewportController)
    {
        m_viewportController->setToolIndex(index);
    }

    const DicomGraphicsView::ToolMode toolMode = m_measurementController.toolModeForIndex(index);
    m_view->setToolMode(toolMode);
    if (toolMode == DicomGraphicsView::ToolMode::Pan)
    {
        statusBar()->clearMessage();
    }
}

void DicomMainWindow::onWindowLevelChanged(int value)
{
    m_viewportController->setWindowLevel(value);
    if (m_presetComboBox && m_presetComboBox->currentIndex() != 0)
    {
        m_viewportController->resetPreset();
        m_presetComboBox->setCurrentIndex(0);
    }
    applyImageAdjustments();
}

void DicomMainWindow::onWindowWidthChanged(int value)
{
    m_viewportController->setWindowWidth(value);
    if (m_presetComboBox && m_presetComboBox->currentIndex() != 0)
    {
        m_viewportController->resetPreset();
        m_presetComboBox->setCurrentIndex(0);
    }
    applyImageAdjustments();
}

void DicomMainWindow::onPresetChanged(int index)
{
    if (!m_windowLevelSlider || !m_windowWidthSlider)
    {
        return;
    }

    if (!m_viewportController || !m_viewportController->currentImage() ||
        !m_viewportController->currentImage()->hasRawPixels())
    {
        return;
    }
    if (!m_viewportController->applyPreset(index))
    {
        return;
    }

    m_windowLevelSlider->blockSignals(true);
    m_windowWidthSlider->blockSignals(true);
    m_windowLevelSlider->setValue(m_viewportController->currentWindowLevel());
    m_windowWidthSlider->setValue(m_viewportController->currentWindowWidth());
    m_windowLevelSlider->blockSignals(false);
    m_windowWidthSlider->blockSignals(false);
    applyImageAdjustments();
}

void DicomMainWindow::onDistanceMeasurementRequested(const QPoint& startPixel, const QPoint& endPixel)
{
    if (!m_viewportController || !m_viewportController->currentImage() ||
        !m_viewportController->currentImage()->hasRawPixels() || !m_view)
    {
        return;
    }

    const auto result = m_measurementController.createDistanceResult(
        *m_viewportController->currentImage(),
        startPixel,
        endPixel);
    m_view->showDistanceMeasurement(result.startScenePos, result.endScenePos, result.label);
    statusBar()->showMessage("Distance: " + result.label, 5000);
}

void DicomMainWindow::onPixelProbeRequested(const QPoint& pixelPos)
{
    if (!m_viewportController || !m_viewportController->currentImage() ||
        !m_viewportController->currentImage()->hasRawPixels() || !m_view)
    {
        return;
    }

    const auto result = m_measurementController.createProbeResult(
        *m_viewportController->currentImage(),
        m_viewportController->currentSeries().get(),
        pixelPos);
    m_view->showPixelProbe(result.scenePos, result.label);
    statusBar()->showMessage("Pixel Probe: " + result.label, 5000);
}

void DicomMainWindow::onAngleMeasurementRequested(const QPoint& startPixel, const QPoint& vertexPixel, const QPoint& endPixel)
{
    if (!m_view)
    {
        return;
    }

    const auto result = m_measurementController.createAngleResult(startPixel, vertexPixel, endPixel);
    m_view->showAngleMeasurement(result.startScenePos, result.vertexScenePos, result.endScenePos, result.label);
    statusBar()->showMessage("Angle: " + result.label, 5000);
}
