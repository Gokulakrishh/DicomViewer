#include "DicomMainWindow.h"

#include <QAction>
#include <QBuffer>
#include <QFileDialog>
#include <QFileInfo>
#include <QDate>
#include <QCheckBox>
#include <QDockWidget>
#include <QDialog>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QFutureWatcher>
#include <QPlainTextEdit>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMetaObject>
#include <QPointer>
#include <QSet>
#include <QSlider>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStatusBar>
#include <QStringList>
#include <QTextCursor>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent/QtConcurrent>
#include <algorithm>
#include <cmath>

#include "AI/GeminiAiAssistantService.h"
#include "AI/IAiAssistantService.h"
#include "AI/QtHttpAiServerClient.h"
#include "Utilities/AiApiKeyDialog.h"
#include "Utilities/IAppConfigService.h"
#include "Utilities/LoadingDialog.h"
#include "Utilities/IWarningDialogService.h"
#include "Database/PostgreService.h"
#include "FileHandling/GDCMFileHandling.h"
#include "Model/MedicalImage.h"
#include "Services/VolumeBuilder.h"
#include "Services/WindowingAnalysisService.h"

constexpr int kFilePathRole = Qt::UserRole + 1;
constexpr int kSeriesInstanceUidRole = Qt::UserRole + 2;
constexpr int kPatientNameRole = Qt::UserRole + 3;
constexpr int kPatientDobRole = Qt::UserRole + 4;
constexpr int kDoctorNameRole = Qt::UserRole + 5;
constexpr int kModalityRole = Qt::UserRole + 6;
constexpr int kStudyDateRole = Qt::UserRole + 7;
constexpr int kSearchTextRole = Qt::UserRole + 8;
constexpr int kPatientIdRole = Qt::UserRole + 9;
constexpr int kStudyInstanceUidRole = Qt::UserRole + 10;
constexpr int kNodeTypeRole = Qt::UserRole + 11;
constexpr int kChildrenLoadedRole = Qt::UserRole + 12;
constexpr auto kNodeTypePatient = "patient";
constexpr auto kNodeTypeStudy = "study";
constexpr auto kNodeTypeSeries = "series";

DicomMainWindow::DicomMainWindow(
    std::unique_ptr<IAppConfigService> appConfigService,
    std::unique_ptr<IAdvancedViewerLauncher> advancedViewerLauncher,
    std::unique_ptr<IWarningDialogService> warningDialogService,
    QWidget* parent)
    : QMainWindow(parent),
      m_ui(new Ui::DicomMainWindow),
      m_appConfigService(std::move(appConfigService)),
      m_advancedViewerLauncher(std::move(advancedViewerLauncher)),
      m_warningDialogService(std::move(warningDialogService))
{
    if (m_warningDialogService)
    {
        m_warningDialogService->setParentWidget(this);
    }
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
    m_searchLineEdit->setPlaceholderText("Filter loaded tree...");
    m_globalSearchLineEdit = new QLineEdit(previewContainer);
    m_globalSearchLineEdit->setPlaceholderText("Global DB search by patient, doctor, modality, series...");
    previewLayout->addWidget(m_searchLineEdit);
    previewLayout->addWidget(m_globalSearchLineEdit);
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

    m_ui->contrastVerticalSlider->hide();
    m_ui->imageVerticalSlider->hide();
    m_ui->cineCheckBox->hide();
    m_ui->cineLabel->hide();
    m_ui->viewerControlsSeparator->hide();

    m_view->setSliceNavigationState(0, 0);
    m_view->setCineAvailable(false);
    m_view->setCinePlaying(false);

    m_cineTimer = new QTimer(this);
    m_cineTimer->setInterval(100);

    m_gdcmHandler = std::make_unique<GDCMFileHandling>();
    m_volumeBuilder = std::make_unique<VolumeBuilder>(m_appConfigService->loadVolumeValidationSettings());
    m_windowingAnalysisService = std::make_unique<WindowingAnalysisService>();
    m_viewportController = std::make_unique<DicomViewportController>(
        m_gdcmHandler.get(),
        &m_renderService,
        m_windowingAnalysisService.get(),
        this);
    rebuildAiAssistantService();
    m_databaseService = std::make_unique<PostgreService>(m_appConfigService->loadDatabaseSettings());
    setupAiDock();
    m_aiResponseWatcher = new QFutureWatcher<AiChatResponse>(this);
    m_folderImportWatcher = new QFutureWatcher<FolderImportResult>(this);

    const bool databaseInitialized = m_databaseService->initialize();
    if (!databaseInitialized)
    {
        const QString warningMessage =
            m_databaseService->lastErrorText() + " Config file: " + m_appConfigService->configFilePath();
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

    m_aiPreferencesAction = new QAction("Preferences...", this);
    m_aiPreferencesAction->setMenuRole(QAction::PreferencesRole);
    m_ui->fileMenu->addAction(m_aiPreferencesAction);

    m_openMprAction = new QAction("Open MPR Viewer", this);
    m_ui->viewMenu->addAction(m_openMprAction);

    connect(m_openFileAction, &QAction::triggered, this, &DicomMainWindow::openImage);
    connect(m_openFolderAction, &QAction::triggered, this, &DicomMainWindow::openFolder);
    connect(m_aiPreferencesAction, &QAction::triggered, this, &DicomMainWindow::openAiPreferences);
    connect(m_openMprAction, &QAction::triggered, this, &DicomMainWindow::openMprViewer);
}

void DicomMainWindow::setupConnections()
{
    if (m_treeView)
    {
        connect(m_treeView, &QTreeView::clicked, this, &DicomMainWindow::onHierarchyItemActivated);
        connect(m_treeView, &QTreeView::expanded, this, &DicomMainWindow::onHierarchyItemExpanded);
    }

    if (m_searchLineEdit)
    {
        connect(m_searchLineEdit, &QLineEdit::textChanged, this, &DicomMainWindow::onLocalSearchTextChanged);
    }

    if (m_globalSearchLineEdit)
    {
        connect(m_globalSearchLineEdit, &QLineEdit::textChanged, this, &DicomMainWindow::onGlobalSearchTextChanged);
    }

    if (m_view)
    {
        connect(m_view, &DicomGraphicsView::toolModeSelected, this, &DicomMainWindow::onToolChanged);
        connect(m_view, &DicomGraphicsView::sliceIndexSelected, this, &DicomMainWindow::onImageSliderValueChanged);
        connect(m_view, &DicomGraphicsView::cinePlaybackToggled, this, &DicomMainWindow::onCineToggled);
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

    if (m_autoWindowPresetComboBox)
    {
        connect(
            m_autoWindowPresetComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &DicomMainWindow::onAutoWindowPresetChanged);
    }

    if (m_openMprButton)
    {
        connect(m_openMprButton, &QPushButton::clicked, this, &DicomMainWindow::openMprViewer);
    }

    if (m_aiAskButton)
    {
        connect(m_aiAskButton, &QPushButton::clicked, this, &DicomMainWindow::onAskAiClicked);
    }

    if (m_aiResponseWatcher)
    {
        connect(m_aiResponseWatcher, &QFutureWatcher<AiChatResponse>::finished, this, &DicomMainWindow::onAiRequestFinished);
    }

    if (m_folderImportWatcher)
    {
        connect(m_folderImportWatcher, &QFutureWatcher<FolderImportResult>::finished, this, &DicomMainWindow::onFolderImportFinished);
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

    const QString globalSearchText = m_globalSearchLineEdit ? m_globalSearchLineEdit->text().trimmed() : QString();
    const QList<DatabaseService::PatientPtr> patients = m_databaseService->getAllPatients(globalSearchText);
    for (const auto& patient : patients)
    {
        addPatientToTree(patient);
    }

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
    patientItem->setData(kNodeTypePatient, kNodeTypeRole);
    patientItem->setData(false, kChildrenLoadedRole);
    patientItem->setData(patient->patientId(), kPatientIdRole);
    patientItem->setData(patient->patientName(), kPatientNameRole);
    patientItem->setData(patient->dateOfBirth(), kPatientDobRole);
    patientItem->setData(
        QString("%1 %2 %3").arg(patient->patientId(), patient->patientName(), patient->dateOfBirth()),
        kSearchTextRole);
    m_treeModel->invisibleRootItem()->appendRow(patientItem);
    patientItem->appendRow(new QStandardItem("Loading..."));
}

void DicomMainWindow::addStudyToTree(
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
    studyItem->setData(kNodeTypeStudy, kNodeTypeRole);
    studyItem->setData(false, kChildrenLoadedRole);
    studyItem->setData(patient->patientId(), kPatientIdRole);
    studyItem->setData(study->studyInstanceUid(), kStudyInstanceUidRole);
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
    studyItem->appendRow(new QStandardItem("Loading..."));
    patientItem->appendRow(studyItem);
}

void DicomMainWindow::addSeriesToTree(
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
    seriesItem->setData(kNodeTypeSeries, kNodeTypeRole);
    seriesItem->setData(true, kChildrenLoadedRole);
    seriesItem->setData(series->seriesInstanceUid(), kSeriesInstanceUidRole);
    seriesItem->setData(patient->patientId(), kPatientIdRole);
    seriesItem->setData(study->studyInstanceUid(), kStudyInstanceUidRole);
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

            if (series->imageCount() > 0)
            {
                seriesItem->setText(seriesLabel + QString(" | %1 slices").arg(series->imageCount()));
                seriesItem->setData(series->representativeFilePath(), kFilePathRole);
            }
    studyItem->appendRow(seriesItem);
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

    if (m_view)
    {
        m_view->setSliceNavigationState(0, 0);
        m_view->setCineAvailable(false);
        m_view->setCinePlaying(false);
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
    if (!series || series->previewPixmap().isNull())
    {
        updatePreviewPane(QPixmap());
        return;
    }

    updatePreviewPane(series->previewPixmap());
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

    if (m_view)
    {
        m_view->setCineAvailable(hasPlayableSeries);
        if (!hasPlayableSeries)
        {
            if (m_viewportController)
            {
                m_viewportController->setCinePlaying(false);
            }
            m_view->setCinePlaying(false);
        }
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
    const bool cinePlaying = m_viewportController->isCinePlaying();
    if (!m_viewportController->prepareCurrentSeriesImage(cinePlaying, &failedFilePath))
    {
        const QString warningMessage = "Failed to load image: " + QFileInfo(failedFilePath).fileName();
        statusBar()->showMessage(warningMessage, 4000);
        m_warningDialogService->showWarning("Image Loading", warningMessage);
        m_view->clearImage();
        return;
    }

    if (!cinePlaying)
    {
        updateImageControlsState(false);
    }
    applyImageAdjustments();

    const auto currentSeries = m_viewportController->currentSeries();
    if (m_view)
    {
        m_view->setSliceNavigationState(
            m_viewportController->currentImageIndex(),
            currentSeries ? static_cast<int>(currentSeries->images().size()) : 0);
    }
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

    if (m_view)
    {
        m_view->setSliceNavigationState(m_viewportController->currentImageIndex(), imageCount);
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
        m_view->setSliceNavigationState(0, 1);
        m_view->setCineAvailable(false);
        m_view->setCinePlaying(false);
        updateImageControlsState(true);
        updatePreviewPane(singleImage->pixmap());
        applyImageAdjustments();
        return;
    }

    std::shared_ptr<MedicalImage> image = std::move(loadedImage);
    m_view->setImage(std::move(image));
    statusBar()->showMessage("Loaded: " + QFileInfo(filePath).fileName(), 4000);
}

void DicomMainWindow::openMprViewer()
{
    if (!m_viewportController || !m_volumeBuilder || !m_advancedViewerLauncher)
    {
        return;
    }

    const auto currentSeries = m_viewportController->currentSeries();
    if (!currentSeries || currentSeries->images().size() < 2)
    {
        m_warningDialogService->showWarning("MPR Viewer", "Select a multi-slice series before opening MPR.");
        return;
    }

    LoadingDialog loadingDialog(this);
    loadingDialog.show("Loading MPR", "Preparing MPR viewer...");

    for (auto& image : currentSeries->images())
    {
        if (!image)
        {
            continue;
        }

        if (!m_viewportController->ensureImageLoaded(*image))
        {
            loadingDialog.close();
            m_warningDialogService->showWarning(
                "MPR Viewer",
                "Failed to fully load all slices required for MPR.");
            return;
        }
    }

    std::shared_ptr<IVolumeData> volume;
    try
    {
        loadingDialog.setMessage("Building MPR volume...");
        volume = m_volumeBuilder->buildFromSeries(*currentSeries);
    }
    catch (const std::exception& exception)
    {
        loadingDialog.close();
        m_warningDialogService->showWarning("MPR Viewer", QString("Failed to build volume: %1").arg(exception.what()));
        return;
    }

    if (!volume)
    {
        loadingDialog.close();
        m_warningDialogService->showWarning("MPR Viewer", "Unable to build a volume from the selected series.");
        return;
    }

    QString title = "MPR Viewer";
    if (!currentSeries->seriesDescription().trimmed().isEmpty())
    {
        title += " - " + currentSeries->seriesDescription().trimmed();
    }
    else if (!currentSeries->modality().trimmed().isEmpty())
    {
        title += " - " + currentSeries->modality().trimmed();
    }

    QWidget* viewer = m_advancedViewerLauncher->showMprVolume(
        std::move(volume),
        title,
        m_viewportController->currentWindowLevel(),
        m_viewportController->currentWindowWidth(),
        this);
    loadingDialog.close();
    if (viewer)
    {
        viewer->raise();
        viewer->activateWindow();
    }
}

void DicomMainWindow::openAiPreferences()
{
    if (!m_appConfigService)
    {
        return;
    }

    QString errorMessage;
    AiApiKeyDialog dialog(this);
    dialog.setApiKey(m_appConfigService->loadAiApiKey(&errorMessage));
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    if (!m_appConfigService->saveAiApiKey(dialog.apiKey(), &errorMessage))
    {
        m_warningDialogService->showWarning("AI Preferences", errorMessage);
        return;
    }

    rebuildAiAssistantService();
    refreshAiDockState();
    statusBar()->showMessage("AI API key updated.", 4000);
}

void DicomMainWindow::rebuildAiAssistantService()
{
    m_aiAssistantService.reset();
    if (!m_appConfigService)
    {
        return;
    }

    const AiServiceSettings aiSettings = m_appConfigService->loadAiServiceSettings();
    if (aiSettings.provider == AiProvider::Gemini)
    {
        m_aiAssistantService = std::make_unique<GeminiAiAssistantService>(
            aiSettings,
            std::make_shared<QtHttpAiServerClient>());
    }
}

void DicomMainWindow::refreshAiDockState()
{
    if (!m_aiDock)
    {
        return;
    }

    const bool aiEnabled = m_aiAssistantService && m_aiAssistantService->isConfigured();
    m_aiDock->setEnabled(aiEnabled);

    if (!aiEnabled)
    {
        if (m_aiModelComboBox)
        {
            m_aiModelComboBox->clear();
            m_aiModelComboBox->setEnabled(false);
        }
        if (m_aiChatHistoryEdit)
        {
            m_aiChatHistoryEdit->setPlainText(
                "AI service is not configured. Add your API key in Preferences.");
            m_aiHistoryShowingStatusMessage = true;
        }
        return;
    }

    if (!m_aiModelComboBox)
    {
        return;
    }

    QString modelError;
    const auto aiSettings = m_appConfigService ? m_appConfigService->loadAiServiceSettings() : AiServiceSettings{};
    const QVector<AiModelInfo> availableModels = m_aiAssistantService->availableModels(&modelError);
    if (m_aiHistoryShowingStatusMessage && m_aiChatHistoryEdit)
    {
        m_aiChatHistoryEdit->clear();
        m_aiHistoryShowingStatusMessage = false;
    }
    m_aiModelComboBox->clear();
    m_aiModelComboBox->setEnabled(true);

    for (const auto& modelInfo : availableModels)
    {
        const QString label = modelInfo.supportsVision
                                  ? QString("%1 (Vision)").arg(modelInfo.displayName)
                                  : modelInfo.displayName;
        m_aiModelComboBox->addItem(label, modelInfo.id);
    }

    if (m_aiModelComboBox->count() > 0)
    {
        int selectedIndex = 0;
        if (!aiSettings.model.trimmed().isEmpty())
        {
            const int configuredIndex = m_aiModelComboBox->findData(aiSettings.model.trimmed());
            if (configuredIndex >= 0)
            {
                selectedIndex = configuredIndex;
            }
        }
        m_aiModelComboBox->setCurrentIndex(selectedIndex);
    }
    else
    {
        m_aiModelComboBox->addItem("No compatible models found");
        m_aiModelComboBox->setEnabled(false);
        if (m_aiChatHistoryEdit)
        {
            const QString details = modelError.trimmed().isEmpty()
                                        ? QString("No compatible AI models were returned.")
                                        : modelError.trimmed();
            m_aiChatHistoryEdit->setPlainText(QString("AI model discovery failed.\n%1").arg(details));
            m_aiHistoryShowingStatusMessage = true;
        }
    }
}

void DicomMainWindow::appendAiMessage(const QString& speaker, const QString& message)
{
    if (!m_aiChatHistoryEdit)
    {
        return;
    }

    const QString safeSpeaker = normalizedAiSpeakerName(speaker).toHtmlEscaped();
    const QString safeMessage = message.trimmed().toHtmlEscaped().replace('\n', "<br/>");
    const bool isUser = speaker.trimmed().compare("You", Qt::CaseInsensitive) == 0;
    const QString alignment = isUser ? "left" : "right";
    const QString bubbleColor = isUser ? "#E7F0FF" : "#D9ECFF";
    const QString bubbleHtml =
        QString(
            "<div style='text-align:%1; margin:8px 0;'>"
            "<div style='display:inline-block; max-width:78%%; background:%2; color:#183247; "
            "border-radius:12px; padding:8px 12px; text-align:left;'>"
            "<div style='font-weight:600; margin-bottom:4px;'>%3</div>"
            "<div>%4</div>"
            "</div>"
            "</div>")
            .arg(alignment, bubbleColor, safeSpeaker, safeMessage);

    QTextCursor cursor = m_aiChatHistoryEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    if (!m_aiChatHistoryEdit->document()->isEmpty())
    {
        cursor.insertHtml("<div style='height:6px;'></div>");
    }
    cursor.insertHtml(bubbleHtml);
    m_aiChatHistoryEdit->setTextCursor(cursor);
    m_aiHistoryShowingStatusMessage = false;
}

QString DicomMainWindow::normalizedAiSpeakerName(const QString& speaker) const
{
    if (speaker.trimmed().compare("AI", Qt::CaseInsensitive) != 0)
    {
        return speaker.trimmed();
    }

    if (m_aiModelComboBox)
    {
        const QString selectedModelId = m_aiModelComboBox->currentData().toString().trimmed();
        if (!selectedModelId.isEmpty())
        {
            return selectedModelId.section('/', -1);
        }
    }

    return QString("Assistant");
}

QString DicomMainWindow::buildAiContextPrompt() const
{
    QStringList contextLines;
    if (m_viewportController)
    {
        if (const auto* currentImage = m_viewportController->currentImage())
        {
            contextLines << QString("Current file: %1").arg(currentImage->filePath());
            contextLines << QString("Image size: %1 x %2").arg(currentImage->width()).arg(currentImage->height());
            contextLines << QString("Window level: %1").arg(m_viewportController->currentWindowLevel());
            contextLines << QString("Window width: %1").arg(m_viewportController->currentWindowWidth());
        }

        if (const auto currentSeries = m_viewportController->currentSeries())
        {
            contextLines << QString("Series description: %1").arg(currentSeries->seriesDescription());
            contextLines << QString("Modality: %1").arg(currentSeries->modality());
            contextLines << QString("Current slice index: %1").arg(m_viewportController->currentImageIndex());
            contextLines << QString("Slice count: %1").arg(static_cast<int>(currentSeries->images().size()));
        }
    }

    if (m_patientNameValueLabel)
    {
        contextLines << QString("Patient name: %1").arg(m_patientNameValueLabel->text());
    }
    if (m_patientDobValueLabel)
    {
        contextLines << QString("Patient DOB: %1").arg(m_patientDobValueLabel->text());
    }
    if (m_doctorValueLabel)
    {
        contextLines << QString("Doctor: %1").arg(m_doctorValueLabel->text());
    }

    return contextLines.join('\n');
}

AiChatRequest DicomMainWindow::buildAiChatRequest(const QString& userPrompt, bool includeCurrentImage) const
{
    AiChatRequest request;
    if (!m_appConfigService)
    {
        return request;
    }

    const auto aiSettings = m_appConfigService->loadAiServiceSettings();
    request.generationOptions.reasoningLevel =
        m_aiReasoningComboBox
            ? static_cast<AiReasoningLevel>(m_aiReasoningComboBox->currentIndex())
            : aiSettings.defaultReasoningLevel;
    if (m_aiModelComboBox)
    {
        request.generationOptions.modelOverride = m_aiModelComboBox->currentData().toString().trimmed();
    }
    request.generationOptions.maxOutputTokens = aiSettings.maxOutputTokens;

    AiChatMessage systemMessage;
    systemMessage.role = AiChatRole::System;
    systemMessage.content =
        "You are an educational radiology assistant for medical students. "
        "Explain clearly, stay non-diagnostic, and explicitly state uncertainty when needed.";
    request.messages.append(systemMessage);

    AiChatMessage userMessage;
    userMessage.role = AiChatRole::User;
    const QString contextPrompt = buildAiContextPrompt();
    userMessage.content = contextPrompt.isEmpty()
                              ? userPrompt.trimmed()
                              : QString("Context:\n%1\n\nQuestion:\n%2").arg(contextPrompt, userPrompt.trimmed());

    if (includeCurrentImage && m_viewportController)
    {
        const auto renderedImage = m_viewportController->renderCurrentImage();
        if (renderedImage && !renderedImage->pixmap().isNull())
        {
            QByteArray imageBytes;
            QBuffer buffer(&imageBytes);
            buffer.open(QIODevice::WriteOnly);
            renderedImage->pixmap().toImage().save(&buffer, "PNG");
            if (!imageBytes.isEmpty())
            {
                userMessage.imageAttachments.append({QStringLiteral("image/png"), imageBytes});
            }
        }
    }

    request.messages.append(userMessage);
    return request;
}

void DicomMainWindow::onAskAiClicked()
{
    if (!m_aiAssistantService || !m_aiAssistantService->isConfigured() || !m_aiPromptEdit || !m_aiAskButton || !m_aiResponseWatcher)
    {
        return;
    }

    const QString prompt = m_aiPromptEdit->toPlainText().trimmed();
    if (prompt.isEmpty())
    {
        return;
    }

    appendAiMessage("You", prompt);
    m_aiPromptEdit->clear();
    m_aiAskButton->setEnabled(false);

    const AiChatRequest request = buildAiChatRequest(
        prompt,
        m_aiIncludeImageCheckBox && m_aiIncludeImageCheckBox->isChecked());
    IAiAssistantService* aiAssistant = m_aiAssistantService.get();
    m_aiResponseWatcher->setFuture(QtConcurrent::run([aiAssistant, request]() {
        return aiAssistant->sendChat(request);
    }));
}

void DicomMainWindow::onAiRequestFinished()
{
    if (!m_aiResponseWatcher)
    {
        return;
    }

    if (m_aiAskButton)
    {
        m_aiAskButton->setEnabled(true);
    }

    const AiChatResponse response = m_aiResponseWatcher->result();
    QString assistantName = response.modelName.trimmed();
    if (!assistantName.isEmpty())
    {
        assistantName = assistantName.section('/', -1);
    }
    else if (!response.providerName.trimmed().isEmpty())
    {
        assistantName = response.providerName.trimmed();
    }
    else
    {
        assistantName = "AI";
    }

    if (response.success)
    {
        appendAiMessage(assistantName, response.answer);
    }
    else
    {
        appendAiMessage(assistantName, QString("Request failed: %1").arg(response.errorMessage));
    }
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

    if (!m_appConfigService || m_folderImportWatcher == nullptr)
    {
        statusBar()->showMessage("Folder import is not available.", 4000);
        m_warningDialogService->showWarning("Folder Import", "Folder import is not available.");
        return;
    }

    if (m_folderImportWatcher->isRunning())
    {
        statusBar()->showMessage("Folder import is already running.", 4000);
        return;
    }

    if (m_folderImportLoadingDialog)
    {
        m_folderImportLoadingDialog->close();
        m_folderImportLoadingDialog->deleteLater();
        m_folderImportLoadingDialog = nullptr;
    }

    m_folderImportLoadingDialog = new LoadingDialog(this);
    m_folderImportLoadingDialog->show("Folder Import", "Scanning and importing DICOM folder...");
    m_folderImportLoadingDialog->setProgressRange(0, 100);
    m_folderImportLoadingDialog->setProgressValue(0);

    const DatabaseSettings databaseSettings = m_appConfigService->loadDatabaseSettings();
    const QString folderPathCopy = folderPath;
    QPointer<LoadingDialog> loadingDialog(m_folderImportLoadingDialog);
    QPointer<DicomMainWindow> window(this);
    m_folderImportWatcher->setFuture(QtConcurrent::run([databaseSettings, folderPathCopy, loadingDialog, window]() {
        FolderImportResult result;
        result.folderName = QFileInfo(folderPathCopy).fileName();
        const auto reportProgress = [loadingDialog, window](int value, const QString& message) {
            if (!window)
            {
                return;
            }

            QMetaObject::invokeMethod(
                window,
                [loadingDialog, window, value, message]() {
                    if (window)
                    {
                        window->statusBar()->showMessage(message);
                    }
                    if (loadingDialog)
                    {
                        loadingDialog->setMessage(message);
                        loadingDialog->setProgressValue(value);
                    }
                },
                Qt::QueuedConnection);
        };

        GDCMFileHandling fileHandling;
        PostgreService databaseService(databaseSettings);
        result.initializeSucceeded = databaseService.initialize();
        if (!result.initializeSucceeded)
        {
            result.errorMessage = databaseService.lastErrorText();
            return result;
        }

        reportProgress(0, "Scanning DICOM files... 0%");
        const FileHandling::PatientList patients = fileHandling.loadDicomFolder(
            folderPathCopy,
            [&reportProgress](int current, int total) {
                const int percent = total > 0 ? std::clamp((current * 70) / total, 0, 70) : 0;
                reportProgress(percent, QString("Scanning DICOM files... %1%").arg(percent));
            });
        result.foundImportableDicom = !patients.isEmpty();
        if (!result.foundImportableDicom)
        {
            reportProgress(100, "No importable DICOM files found.");
            return result;
        }

        const int patientCount = patients.size();
        int importedIndex = 0;
        for (const auto& patient : patients)
        {
            if (databaseService.savePatient(patient))
            {
                ++result.importedPatientCount;
            }
            else
            {
                result.hadSaveFailure = true;
            }

            ++importedIndex;
            const int percent = 70 + (patientCount > 0 ? std::clamp((importedIndex * 30) / patientCount, 0, 30) : 30);
            reportProgress(
                percent,
                QString("Importing patients... %1/%2").arg(importedIndex).arg(patientCount));
        }

        if (result.hadSaveFailure && result.importedPatientCount == 0)
        {
            result.errorMessage = databaseService.lastErrorText();
        }

        reportProgress(100, QString("Imported folder %1").arg(result.folderName));

        return result;
    }));
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
    const QString nodeType = item->data(kNodeTypeRole).toString();
    if (nodeType == kNodeTypePatient && m_databaseService)
    {
        updatePreviewPane(m_databaseService->getPreviewForPatient(item->data(kPatientIdRole).toString()));
    }
    else if (nodeType == kNodeTypeStudy && m_databaseService)
    {
        updatePreviewPane(m_databaseService->getPreviewForStudy(item->data(kStudyInstanceUidRole).toString()));
    }
    else if (nodeType == kNodeTypeSeries && m_databaseService)
    {
        updatePreviewPane(m_databaseService->getPreviewForSeries(item->data(kSeriesInstanceUidRole).toString()));
    }
    else
    {
        updatePreviewPane(QPixmap());
    }

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

void DicomMainWindow::onHierarchyItemExpanded(const QModelIndex& index)
{
    if (!index.isValid() || !m_treeModel || !m_databaseService)
    {
        return;
    }

    QStandardItem* item = m_treeModel->itemFromIndex(index);
    if (!item || item->data(kChildrenLoadedRole).toBool())
    {
        return;
    }

    const QString nodeType = item->data(kNodeTypeRole).toString();
    if (nodeType == kNodeTypePatient)
    {
        const QString patientId = item->data(kPatientIdRole).toString();
        if (patientId.isEmpty())
        {
            item->removeRows(0, item->rowCount());
            item->setData(true, kChildrenLoadedRole);
            return;
        }

        auto patient = std::make_shared<Patient>();
        patient->setPatientId(patientId);
        patient->setPatientName(item->data(kPatientNameRole).toString());
        patient->setDateOfBirth(item->data(kPatientDobRole).toString());

        item->removeRows(0, item->rowCount());
        const QString globalSearchText = m_globalSearchLineEdit ? m_globalSearchLineEdit->text().trimmed().toLower() : QString();
        const bool patientMatchesGlobalSearch =
            globalSearchText.isEmpty() || item->data(kSearchTextRole).toString().toLower().contains(globalSearchText);
        const QList<DatabaseService::StudyPtr> studies =
            m_databaseService->getStudiesForPatient(patientId, patientMatchesGlobalSearch ? QString() : globalSearchText);
        for (const auto& study : studies)
        {
            addStudyToTree(item, patient, study);
        }
        item->setData(true, kChildrenLoadedRole);
    }
    else if (nodeType == kNodeTypeStudy)
    {
        const QString patientId = item->data(kPatientIdRole).toString();
        const QString studyInstanceUid = item->data(kStudyInstanceUidRole).toString();
        if (patientId.isEmpty() || studyInstanceUid.isEmpty())
        {
            item->removeRows(0, item->rowCount());
            item->setData(true, kChildrenLoadedRole);
            return;
        }

        auto patient = std::make_shared<Patient>();
        patient->setPatientId(patientId);
        patient->setPatientName(item->data(kPatientNameRole).toString());
        patient->setDateOfBirth(item->data(kPatientDobRole).toString());

        auto study = std::make_shared<Study>();
        study->setStudyInstanceUid(studyInstanceUid);
        study->setDoctorName(item->data(kDoctorNameRole).toString());
        study->setStudyDate(item->data(kStudyDateRole).toString());

        item->removeRows(0, item->rowCount());
        const QString globalSearchText = m_globalSearchLineEdit ? m_globalSearchLineEdit->text().trimmed().toLower() : QString();
        const bool studyMatchesGlobalSearch =
            globalSearchText.isEmpty() || item->data(kSearchTextRole).toString().toLower().contains(globalSearchText);
        const QList<DatabaseService::SeriesPtr> seriesList =
            m_databaseService->getSeriesForStudy(studyInstanceUid, studyMatchesGlobalSearch ? QString() : globalSearchText);
        for (const auto& series : seriesList)
        {
            addSeriesToTree(item, patient, study, series);
        }
        item->setData(true, kChildrenLoadedRole);
    }
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

    onImageSliderValueChanged(nextIndex);
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
        updateImageControlsState(false);
    }
}

void DicomMainWindow::advanceCinePlayback()
{
    if (!m_viewportController || !m_viewportController->hasPlayableSeries())
    {
        return;
    }

    onImageSliderValueChanged(m_viewportController->nextCineIndex());
}

void DicomMainWindow::onLocalSearchTextChanged(const QString& text)
{
    applyTreeFilter(text);
}

void DicomMainWindow::onGlobalSearchTextChanged(const QString& text)
{
    Q_UNUSED(text);
    refreshHierarchyForGlobalSearch();
}

void DicomMainWindow::onFolderImportFinished()
{
    if (!m_folderImportWatcher)
    {
        return;
    }

    if (m_folderImportLoadingDialog)
    {
        m_folderImportLoadingDialog->close();
        m_folderImportLoadingDialog->deleteLater();
        m_folderImportLoadingDialog = nullptr;
    }

    const FolderImportResult result = m_folderImportWatcher->result();
    if (!result.initializeSucceeded)
    {
        const QString message = result.errorMessage.trimmed().isEmpty()
                                    ? QString("Failed to initialize PostgreSQL for folder import.")
                                    : result.errorMessage;
        statusBar()->showMessage(message, 6000);
        m_warningDialogService->showWarning("Folder Import", message);
        return;
    }

    if (!result.foundImportableDicom)
    {
        statusBar()->showMessage("No importable DICOM files found in folder.", 5000);
        m_warningDialogService->showWarning("Folder Import", "No importable DICOM files found in the selected folder.");
        return;
    }

    if (result.importedPatientCount == 0)
    {
        const QString message = result.errorMessage.trimmed().isEmpty()
                                    ? QString("The folder was scanned, but no patient hierarchy could be saved into PostgreSQL.")
                                    : result.errorMessage;
        m_warningDialogService->showWarning("Database Import", message);
    }

    refreshHierarchyTree();
    statusBar()->showMessage(
        QString("Imported folder %1 | Patients saved: %2").arg(result.folderName).arg(result.importedPatientCount),
        6000);
}

void DicomMainWindow::refreshHierarchyForGlobalSearch()
{
    refreshHierarchyTree();
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
    dockLayout->addWidget(new QLabel("Manual Preset", dockContentWidget));
    dockLayout->addWidget(m_presetComboBox);

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

    m_autoWindowPresetComboBox = new QComboBox(dockContentWidget);
    m_autoWindowPresetComboBox->addItem("None");
    m_autoWindowPresetComboBox->addItem("General Head CT (1% / 99%)");
    m_autoWindowPresetComboBox->addItem("Brain Focused (5% / 95%)");
    m_autoWindowPresetComboBox->addItem("Bone Heavy (2% / 98%)");
    dockLayout->addWidget(new QLabel("Auto Window Analysis", dockContentWidget));
    dockLayout->addWidget(m_autoWindowPresetComboBox);
    m_openMprButton = new QPushButton("Open MPR", dockContentWidget);
    m_openMprButton->setObjectName("primaryActionButton");
    dockLayout->addWidget(m_openMprButton);
    dockLayout->addStretch();

    m_imageControlsDock->setWidget(dockContentWidget);
    addDockWidget(Qt::RightDockWidgetArea, m_imageControlsDock);
}

void DicomMainWindow::setupAiDock()
{
    m_aiDock = new QDockWidget("Ask AI", this);
    m_aiDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_aiDock->setFeatures(
        QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);

    auto* dockContentWidget = new QWidget(m_aiDock);
    auto* dockLayout = new QVBoxLayout(dockContentWidget);
    dockLayout->setContentsMargins(8, 8, 8, 8);
    dockLayout->setSpacing(8);

    m_aiChatHistoryEdit = new QTextEdit(dockContentWidget);
    m_aiChatHistoryEdit->setReadOnly(true);
    m_aiChatHistoryEdit->setPlaceholderText("AI answers will appear here.");
    m_aiChatHistoryEdit->setMinimumHeight(140);

    m_aiPromptEdit = new QPlainTextEdit(dockContentWidget);
    m_aiPromptEdit->setPlaceholderText(
        "Ask about the current scan, anatomy, modality, windowing, or measurements...");
    m_aiPromptEdit->setMaximumBlockCount(200);
    m_aiPromptEdit->setMinimumHeight(80);

    m_aiIncludeImageCheckBox = new QCheckBox("Include current image", dockContentWidget);
    m_aiIncludeImageCheckBox->setChecked(true);

    m_aiModelComboBox = new QComboBox(dockContentWidget);

    m_aiReasoningComboBox = new QComboBox(dockContentWidget);
    m_aiReasoningComboBox->addItem("Low");
    m_aiReasoningComboBox->addItem("Medium");
    m_aiReasoningComboBox->addItem("High");

    const auto aiSettings = m_appConfigService ? m_appConfigService->loadAiServiceSettings() : AiServiceSettings{};
    m_aiReasoningComboBox->setCurrentIndex(static_cast<int>(aiSettings.defaultReasoningLevel));

    m_aiAskButton = new QPushButton("Ask AI", dockContentWidget);
    m_aiAskButton->setObjectName("primaryActionButton");

    auto* modelLayout = new QVBoxLayout();
    modelLayout->setContentsMargins(0, 0, 0, 0);
    modelLayout->setSpacing(4);
    modelLayout->addWidget(new QLabel("Model", dockContentWidget));
    modelLayout->addWidget(m_aiModelComboBox);

    auto* optionsRowLayout = new QHBoxLayout();
    optionsRowLayout->setContentsMargins(0, 0, 0, 0);
    optionsRowLayout->setSpacing(8);

    auto* reasoningLayout = new QVBoxLayout();
    reasoningLayout->setContentsMargins(0, 0, 0, 0);
    reasoningLayout->setSpacing(4);
    reasoningLayout->addWidget(new QLabel("Reasoning", dockContentWidget));
    reasoningLayout->addWidget(m_aiReasoningComboBox);

    optionsRowLayout->addWidget(m_aiIncludeImageCheckBox, 2, Qt::AlignBottom);
    optionsRowLayout->addLayout(reasoningLayout, 1);

    dockLayout->addWidget(new QLabel("Conversation", dockContentWidget));
    dockLayout->addWidget(m_aiChatHistoryEdit, 3);
    dockLayout->addWidget(new QLabel("Question", dockContentWidget));
    dockLayout->addWidget(m_aiPromptEdit, 1);
    dockLayout->addLayout(modelLayout);
    dockLayout->addLayout(optionsRowLayout);
    dockLayout->addWidget(m_aiAskButton);

    m_aiDock->setWidget(dockContentWidget);
    addDockWidget(Qt::RightDockWidgetArea, m_aiDock);
    if (m_imageControlsDock)
    {
        splitDockWidget(m_imageControlsDock, m_aiDock, Qt::Vertical);
    }

    m_aiDock->setMinimumHeight(260);
    refreshAiDockState();
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
    if (m_autoWindowPresetComboBox)
    {
        m_autoWindowPresetComboBox->blockSignals(true);
        m_autoWindowPresetComboBox->setCurrentIndex(state.autoWindowPresetIndex);
        m_autoWindowPresetComboBox->blockSignals(false);
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

void DicomMainWindow::onToolChanged(DicomGraphicsView::ToolMode toolMode)
{
    if (!m_view)
    {
        return;
    }

    if (m_viewportController)
    {
        m_viewportController->setToolIndex(MeasurementController::toolIndexForMode(toolMode));
    }

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
    if (m_autoWindowPresetComboBox && m_autoWindowPresetComboBox->currentIndex() != 0)
    {
        m_viewportController->resetAutoWindowPreset();
        m_autoWindowPresetComboBox->setCurrentIndex(0);
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
    if (m_autoWindowPresetComboBox && m_autoWindowPresetComboBox->currentIndex() != 0)
    {
        m_viewportController->resetAutoWindowPreset();
        m_autoWindowPresetComboBox->setCurrentIndex(0);
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
    if (m_autoWindowPresetComboBox && m_autoWindowPresetComboBox->currentIndex() != 0)
    {
        m_viewportController->resetAutoWindowPreset();
        m_autoWindowPresetComboBox->blockSignals(true);
        m_autoWindowPresetComboBox->setCurrentIndex(0);
        m_autoWindowPresetComboBox->blockSignals(false);
    }
    applyImageAdjustments();
}

void DicomMainWindow::onAutoWindowPresetChanged(int index)
{
    if (!m_viewportController || !m_windowLevelSlider || !m_windowWidthSlider)
    {
        return;
    }

    if (index == 0)
    {
        m_viewportController->resetAutoWindowPreset();
        return;
    }

    if (!m_viewportController->applyAutoWindowPreset(index))
    {
        return;
    }

    if (m_presetComboBox)
    {
        m_presetComboBox->blockSignals(true);
        m_presetComboBox->setCurrentIndex(0);
        m_presetComboBox->blockSignals(false);
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
