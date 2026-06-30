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
#include <QCoreApplication>
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
#include <QRegularExpression>
#include <QSet>
#include <QSlider>
#include <QStatusBar>
#include <QStyle>
#include <QStringList>
#include <QTextCursor>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent/QtConcurrent>
#include <algorithm>
#include <cmath>

#include "Audit/AuditService.h"
#include "Audit/JsonlAuditSink.h"
#include "AI/GeminiAiAssistantService.h"
#include "AI/IAiAssistantService.h"
#include "AI/QtHttpAiServerClient.h"
#include "Utilities/AiApiKeyDialog.h"
#include "Utilities/AiMessageFormatter.h"
#include "Utilities/ApplicationPaths.h"
#include "Utilities/AppIcons.h"
#include "Utilities/DiagnosticImageRenderer.h"
#include "Utilities/IAppConfigService.h"
#include "Utilities/MemoryManagementDebug.h"
#include "Utilities/LoadingDialog.h"
#include "Utilities/IWarningDialogService.h"
#include "Database/SqliteService.h"
#include "FileHandling/GDCMFileHandling.h"
#include "Model/MedicalImage.h"
#include "ViewerTools/WindowLevelPreset.h"
#include "Services/AnnotationReportService.h"
#include "Services/AdvancedSeriesVolumeService.h"
#include "Services/SeriesPreviewService.h"
#include "Services/ThreeDProfiles/ThreeDProfileSelection.h"
#include "Services/VideoExport/GStreamerVideoExportService.h"
#include "DicomViewerWindow/VideoExportController.h"
#include "AppVersion.h"

namespace
{
constexpr int kDicomWindowPresetBase = 1000;

struct BuiltInViewportPresetMapping
{
    ViewportWindowPreset viewportPreset;
    BuiltInWindowLevelPresetId builtInPreset;
};

constexpr std::array<BuiltInViewportPresetMapping, 4> kBuiltInViewportPresetMappings{{
    {ViewportWindowPreset::Brain, BuiltInWindowLevelPresetId::Brain},
    {ViewportWindowPreset::SoftTissue, BuiltInWindowLevelPresetId::SoftTissue},
    {ViewportWindowPreset::Bone, BuiltInWindowLevelPresetId::Bone},
    {ViewportWindowPreset::Lung, BuiltInWindowLevelPresetId::Lung},
}};

QString buildAdvancedViewerTitle(const QString& viewerName, const Series& selectedSeries)
{
    QString title = viewerName;
    if (!selectedSeries.seriesDescription().trimmed().isEmpty())
    {
        title += " - " + selectedSeries.seriesDescription().trimmed();
    }
    else if (!selectedSeries.modality().trimmed().isEmpty())
    {
        title += " - " + selectedSeries.modality().trimmed();
    }
    return title;
}

QString resolveAuditFilePath()
{
    return ApplicationPaths::auditFilePath();
}

QString buildVolumeGeometryWarningMessage(const QString& viewerName, const QStringList& warnings)
{
    QStringList uniqueWarnings;
    for (const QString& warning : warnings)
    {
        const QString trimmed = warning.trimmed();
        if (!trimmed.isEmpty() && !uniqueWarnings.contains(trimmed))
        {
            uniqueWarnings.append(trimmed);
        }
    }

    QString message = QString(
        "The selected series has DICOM geometry metadata issues. %1 can still be opened, "
        "but slice position, spacing, or orientation may be approximate.\n\n").arg(viewerName);

    for (int index = 0; index < uniqueWarnings.size(); ++index)
    {
        message += QString("%1. %2\n").arg(index + 1).arg(uniqueWarnings.at(index));
    }

    message += QString("\nContinue opening %1 anyway?").arg(viewerName);
    return message;
}
}

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
    setWindowIcon(AppIcons::applicationIcon());
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
    setupViewerToolbar();
    setupStudyBrowserDock();
    setupViewerSurface();
    setupCoreServices();
    setupAnnotationReportDock();
    setupAsyncInfrastructure();
    initializeDatabaseAndTree();
    setupConnections();
}

void DicomMainWindow::setupStudyBrowserDock()
{
    if (m_ui->leftPanelHost)
    {
        m_ui->horizontalLayout->removeWidget(m_ui->leftPanelHost);
        m_ui->leftPanelHost->deleteLater();
    }

    m_studyBrowserDock = new QDockWidget("Study Browser", this);
    m_studyBrowserDock->setObjectName("studyBrowserDock");
    m_studyBrowserDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_studyBrowserDock->setFeatures(
        QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);

    m_treePanel = new DicomTreePanel(m_studyBrowserDock);
    m_treeController = new DicomTreeController(this);
    m_studyBrowserDock->setWidget(m_treePanel);
    m_studyBrowserDock->setMinimumWidth(320);
    addDockWidget(Qt::LeftDockWidgetArea, m_studyBrowserDock);

    if (m_ui->viewMenu)
    {
        m_ui->viewMenu->addAction(m_studyBrowserDock->toggleViewAction());
    }
}

void DicomMainWindow::setupViewerSurface()
{
    m_view = new VtkDiagnosticSliceView(this);
    m_ui->horizontalLayout->replaceWidget(m_ui->viewerHost, m_view);
    m_ui->viewerHost->deleteLater();
    m_ui->horizontalLayout->setSpacing(8);

    m_view->setSliceNavigationState(0, 0);
    m_view->setCineAvailable(false);
    m_view->setCinePlaying(false);
}

void DicomMainWindow::setupCoreServices()
{
    m_cineTimer = new QTimer(this);
    m_cineTimer->setInterval(100);

    m_gdcmHandler = std::make_unique<GDCMFileHandling>();
    m_auditService = std::make_unique<AuditService>();
    m_auditService->addSink(std::make_shared<JsonlAuditSink>(resolveAuditFilePath()));
    m_advancedSeriesVolumeService = std::make_unique<AdvancedSeriesVolumeService>(
        *m_gdcmHandler,
        m_auditService.get());
    m_viewportController = std::make_unique<DicomViewportController>(
        m_gdcmHandler.get(),
        this);
    m_databaseService = std::make_unique<SqliteService>(m_appConfigService->loadDatabaseSettings());
    m_measurementAnnotationStore = std::make_unique<MeasurementAnnotationStore>(*m_databaseService);
    m_annotationReportService = std::make_unique<AnnotationReportService>(*m_databaseService);
    m_seriesPreviewService = std::make_unique<SeriesPreviewService>(*m_databaseService, *m_gdcmHandler);
    m_videoExportController = new VideoExportController(
        std::make_shared<GStreamerVideoExportService>(),
        m_auditService.get(),
        this);
    if (m_treeController)
    {
        m_treeController->setDatabaseService(m_databaseService.get());
        m_treeController->setAnnotationReportService(m_annotationReportService.get());
        m_treeController->setSeriesPreviewService(m_seriesPreviewService.get());
    }
}

void DicomMainWindow::setupAsyncInfrastructure()
{
    m_folderImportWatcher = new QFutureWatcher<FolderImportResult>(this);
}

void DicomMainWindow::initializeDatabaseAndTree()
{
    const bool databaseInitialized = m_databaseService->initialize();
    if (!databaseInitialized)
    {
        const QString warningMessage = m_databaseService->lastErrorText();
        statusBar()->showMessage(warningMessage, 10000);
        m_warningDialogService->showWarning("Local Database", warningMessage);
    }

    refreshHierarchyForGlobalSearch();
}

void DicomMainWindow::setupMenuBar()
{
    m_openFileAction = new QAction("Open File", this);
    m_ui->fileMenu->addAction(m_openFileAction);

    m_openFolderAction = new QAction("Open Folder", this);
    m_ui->fileMenu->addAction(m_openFolderAction);

    m_openMprAction = new QAction("Open MPR Viewer", this);
    m_ui->viewMenu->addAction(m_openMprAction);
    m_openThreeDAction = new QAction("3D", this);
    m_ui->viewMenu->addAction(m_openThreeDAction);

    connect(m_openFileAction, &QAction::triggered, this, &DicomMainWindow::openImage);
    connect(m_openFolderAction, &QAction::triggered, this, &DicomMainWindow::openFolder);
    connect(m_openMprAction, &QAction::triggered, this, &DicomMainWindow::openMprViewer);
    connect(m_openThreeDAction, &QAction::triggered, this, &DicomMainWindow::openThreeDViewer);
}

void DicomMainWindow::setupViewerToolbar()
{
    m_viewerToolBar = addToolBar("Viewer");
    m_viewerToolBar->setMovable(false);
    m_viewerToolPresentation = std::make_unique<ViewerToolPresentation>(
        *m_viewerToolBar,
        this);
    m_viewerToolPresentation->setSelectionChangedCallback([this](std::optional<ViewerToolId>) {
        applyViewerToolSelection();
    });
    m_windowLevelToolAction = m_viewerToolPresentation->action(ViewerToolId::WindowLevel);
    m_zoomToolAction = m_viewerToolPresentation->action(ViewerToolId::Zoom);
    m_panToolAction = m_viewerToolPresentation->action(ViewerToolId::Pan);
    m_distanceMeasurementToolAction = m_viewerToolPresentation->action(ViewerToolId::Distance);
    m_polylineMeasurementToolAction = m_viewerToolPresentation->action(ViewerToolId::Polyline);
    m_angleMeasurementToolAction = m_viewerToolPresentation->action(ViewerToolId::Angle);
    m_rectangleRoiMeasurementToolAction = m_viewerToolPresentation->action(ViewerToolId::RectangleRoi);

    m_viewerToolBar->addSeparator();

    auto* presetLabel = new QLabel("WL/WW Preset:", this);
    m_viewerToolBar->addWidget(presetLabel);

    m_windowLevelPresetComboBox = new QComboBox(this);
    rebuildWindowLevelPresetComboBox();
    m_viewerToolBar->addWidget(m_windowLevelPresetComboBox);
    connect(m_windowLevelPresetComboBox, &QComboBox::currentIndexChanged, this, &DicomMainWindow::onWindowLevelPresetSelected);

    m_viewerToolBar->addSeparator();
    m_exportCineAction = new QAction(
        style()->standardIcon(QStyle::SP_DialogSaveButton),
        QStringLiteral("Export Cine"),
        this);
    m_exportCineAction->setToolTip(QStringLiteral("Export selected XA cine frames"));
    m_exportCineAction->setEnabled(false);
    m_viewerToolBar->addAction(m_exportCineAction);
    connect(m_exportCineAction, &QAction::triggered, this, &DicomMainWindow::exportCurrentCine);
    syncViewerToolbarState();
}

void DicomMainWindow::setupConnections()
{
    if (m_treeController && m_treePanel)
    {
        connect(m_treePanel, &DicomTreePanel::localSearchTextChanged, this, &DicomMainWindow::onLocalSearchTextChanged);
        m_treeController->bindPanel(m_treePanel);
        connect(m_treeController, &DicomTreeController::patientContextSelected, this, &DicomMainWindow::onTreePatientContextSelected);
        connect(m_treeController, &DicomTreeController::seriesSelectionRequested, this, &DicomMainWindow::onTreeSeriesSelectionRequested);
        connect(m_treeController, &DicomTreeController::fileSelectionRequested, this, &DicomMainWindow::onTreeFileSelectionRequested);
    }

    if (m_view)
    {
        connect(m_view, &VtkDiagnosticSliceView::sliceIndexSelected, this, &DicomMainWindow::onImageSliderValueChanged);
        connect(m_view, &VtkDiagnosticSliceView::cinePlaybackToggled, this, &DicomMainWindow::onCineToggled);
        connect(m_view, &VtkDiagnosticSliceView::wheelSliceNavigationRequested, this, &DicomMainWindow::onSliceWheelRequested);
        connect(m_view, &VtkDiagnosticSliceView::windowLevelDragDelta, this, &DicomMainWindow::onWindowLevelDragDelta);
        connect(m_view, &VtkDiagnosticSliceView::sliceAnnotationsChanged, this, &DicomMainWindow::onSliceAnnotationsChanged);
    }

    if (m_cineTimer)
    {
        connect(m_cineTimer, &QTimer::timeout, this, &DicomMainWindow::advanceCinePlayback);
    }

    if (m_aiAskButton)
    {
        connect(m_aiAskButton, &QPushButton::clicked, this, &DicomMainWindow::onAskAiClicked);
    }

    if (m_aiClearButton)
    {
        connect(m_aiClearButton, &QPushButton::clicked, this, &DicomMainWindow::onClearAiConversationClicked);
    }

    if (m_aiResponseWatcher)
    {
        connect(m_aiResponseWatcher, &QFutureWatcher<AiChatResponse>::finished, this, &DicomMainWindow::onAiRequestFinished);
    }

    if (m_folderImportWatcher)
    {
        connect(m_folderImportWatcher, &QFutureWatcher<FolderImportResult>::finished, this, &DicomMainWindow::onFolderImportFinished);
    }

    if (m_videoExportController)
    {
        connect(m_videoExportController, &VideoExportController::runningChanged, this, [this](bool) { updateCineControls(); });
        connect(m_videoExportController, &VideoExportController::exportSucceeded, this, [this](const QString& message) { statusBar()->showMessage(message, 8000); });
        connect(m_videoExportController, &VideoExportController::exportCancelled, this, [this]() { statusBar()->showMessage(QStringLiteral("XA cine export cancelled."), 5000); });
        connect(m_videoExportController, &VideoExportController::exportFailed, this, [this](const QString& message) { statusBar()->showMessage(message, 8000);
                m_warningDialogService->showWarning(QStringLiteral("Export XA Cine"), message);
            });
    }

    if (m_annotationReportPanel)
    {
        connect(m_annotationReportPanel, &AnnotationReportDock::filterChanged, this, &DicomMainWindow::onAnnotationReportFilterChanged);
        connect(m_annotationReportPanel, &AnnotationReportDock::goToSliceRequested, this, &DicomMainWindow::onAnnotationReportGoToSliceRequested);
        connect(m_annotationReportPanel, &AnnotationReportDock::metadataChanged, this, &DicomMainWindow::onAnnotationReportMetadataChanged);
        connect(m_annotationReportPanel, &AnnotationReportDock::deleteRequested, this, &DicomMainWindow::onAnnotationReportDeleteRequested);
    }

    refreshAnnotationReportDock();
}

void DicomMainWindow::clearCurrentSeries()
{
    if (m_viewportController)
    {
        m_viewportController->clear();
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
    if (m_treePanel)
    {
        m_treePanel->updatePreviewPane(QPixmap());
    }

    m_currentPatientName.clear();
    m_currentPatientDob.clear();
    m_currentPatientAge.clear();
    m_currentDoctorName.clear();
    m_currentModality.clear();
    m_currentStudyDate.clear();
    m_activeSliceSopInstanceUid.clear();
    m_activeSliceFrameIndex = 0;
    m_loadedSliceAnnotationIds.clear();
    if (m_view)
    {
        m_view->setSliceAnnotationContext({}, {});
        m_view->loadSliceAnnotations({});
    }
    m_resetViewerFitOnNextImage = true;
    syncViewerToolbarState();
    MemoryManagementDebug::logMainViewerSnapshot(m_viewportController.get(), m_view, "clearCurrentSeries");
}

void DicomMainWindow::updatePatientInfo(
    const QString& patientName,
    const QString& patientDob,
    const QString& doctorName,
    const QString& modality,
    const QString& studyDate)
{
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

    m_currentPatientName = patientName.trimmed();
    m_currentPatientDob = patientDob.trimmed();
    m_currentPatientAge = ageText.trimmed() == "-" ? QString() : ageText.trimmed();
    m_currentDoctorName = doctorName.trimmed();
    m_currentModality = modality.trimmed();
    m_currentStudyDate = studyDate.trimmed();

    refreshAnnotationReportDock();
}

void DicomMainWindow::updateCineControls()
{
    const bool hasPlayableSeries = m_viewportController && m_viewportController->hasPlayableSeries();
    const DicomImage* currentImage = m_viewportController ? m_viewportController->currentImage() : nullptr;
    const bool hasExportableCine =
        currentImage &&
        currentImage->frameCount() > 1 &&
        !currentImage->filePath().trimmed().isEmpty() &&
        (!m_videoExportController || !m_videoExportController->isRunning());

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
    if (m_exportCineAction)
    {
        m_exportCineAction->setEnabled(hasExportableCine);
    }
}

void DicomMainWindow::displayImageInViewer(const DicomImage& image, int windowLevel, int windowWidth)
{
    if (!m_view)
    {
        return;
    }

    m_view->setDicomImage(image, windowLevel, windowWidth, m_resetViewerFitOnNextImage);
    m_resetViewerFitOnNextImage = false;
}

void DicomMainWindow::displayImageInViewer(const std::shared_ptr<DicomImage>& image)
{
    if (!m_view || !image)
    {
        return;
    }

    m_view->setImage(image, m_resetViewerFitOnNextImage);
    m_resetViewerFitOnNextImage = false;
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
        if (cinePlaying)
        {
            return;
        }

        const QString warningMessage = "Failed to load image: " + QFileInfo(failedFilePath).fileName();
        statusBar()->showMessage(warningMessage, 4000);
        m_warningDialogService->showWarning("Image Loading", warningMessage);
        m_view->clearImage();
        return;
    }

    applyImageAdjustments();
    if (cinePlaying)
    {
        if (m_view)
        {
            const auto currentSeries = m_viewportController->currentSeries();
            m_view->setSliceNavigationState(
                m_viewportController->currentImageIndex(),
                currentSeries ? static_cast<int>(currentSeries->images().size()) : 0);
        }
        return;
    }

    loadCurrentSliceAnnotations();

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
    MemoryManagementDebug::logMainViewerSnapshot(m_viewportController.get(), m_view, "displayCurrentSlice");
    updateCineControls();
}

void DicomMainWindow::loadCurrentSliceAnnotations()
{
    if (!m_view || !m_viewportController)
    {
        return;
    }

    const auto currentSeries = m_viewportController->currentSeries();
    const DicomImage* currentImage = m_viewportController->currentImage();
    if (!currentSeries || !currentImage)
    {
        m_activeSliceSopInstanceUid.clear();
        m_activeSliceFrameIndex = 0;
        m_loadedSliceAnnotationIds.clear();
        m_view->setSliceAnnotationContext({}, {});
        m_view->loadSliceAnnotations({});
        refreshAnnotationReportDock();
        return;
    }

    m_view->setSliceAnnotationContext(
        currentSeries->seriesInstanceUid(),
        currentImage->sopInstanceUid(),
        currentImage->frameIndex());
    m_activeSliceSopInstanceUid = currentImage->sopInstanceUid();
    m_activeSliceFrameIndex = currentImage->frameIndex();

    const QList<SliceMeasurementAnnotationRecord> records = m_measurementAnnotationStore
        ? m_measurementAnnotationStore->loadSliceAnnotations(m_activeSliceSopInstanceUid, m_activeSliceFrameIndex)
        : QList<SliceMeasurementAnnotationRecord>{};

    m_loadedSliceAnnotationIds.clear();
    for (const SliceMeasurementAnnotationRecord& record : records)
    {
        m_loadedSliceAnnotationIds.insert(record.measurement.id);
    }

    m_view->loadSliceAnnotations(records);
    refreshAnnotationReportDock();
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
    m_resetViewerFitOnNextImage = true;
    const int imageCount = m_viewportController->imageCount();

    if (m_view)
    {
        m_view->setSliceNavigationState(m_viewportController->currentImageIndex(), imageCount);
    }

    updateCineControls();
    displayCurrentSlice();
    MemoryManagementDebug::logMainViewerSnapshot(m_viewportController.get(), m_view, "loadSeries");
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
        if (m_view)
        {
            m_view->setSliceAnnotationContext({}, {});
            m_view->loadSliceAnnotations({});
        }
        m_resetViewerFitOnNextImage = true;
        m_view->setSliceNavigationState(0, 1);
        m_view->setCineAvailable(false);
        m_view->setCinePlaying(false);
        if (m_treePanel)
        {
            m_treePanel->updatePreviewPane(createDicomPreviewPixmap(*singleImage));
        }
        applyImageAdjustments();
        loadCurrentSliceAnnotations();
        MemoryManagementDebug::logMainViewerSnapshot(m_viewportController.get(), m_view, "loadAndDisplayImage-single");
        return;
    }

    std::shared_ptr<MedicalImage> image = std::move(loadedImage);
    m_resetViewerFitOnNextImage = true;
    m_view->setImage(std::move(image), true);
    statusBar()->showMessage("Loaded: " + QFileInfo(filePath).fileName(), 4000);
    MemoryManagementDebug::logMainViewerSnapshot(m_viewportController.get(), m_view, "loadAndDisplayImage-generic");
}

void DicomMainWindow::openMprViewer()
{
    if (!m_viewportController || !m_advancedSeriesVolumeService || !m_advancedViewerLauncher)
    {
        return;
    }

    const auto selectedSeries = m_viewportController->currentSeries();
    if (!selectedSeries || selectedSeries->images().size() < 2)
    {
        m_warningDialogService->showWarning("MPR Viewer", "Select a multi-slice series before opening MPR.");
        return;
    }

    LoadingDialog loadingDialog(this);
    loadingDialog.show("Loading MPR", "Preparing MPR viewer...");
    loadingDialog.setMessage("Loading diagnostic series...");
    const auto volumeResult = m_advancedSeriesVolumeService->buildDiagnosticVolume(*selectedSeries);
    if (!volumeResult)
    {
        loadingDialog.close();
        m_warningDialogService->showError(volumeResult.error());
        return;
    }
    const VolumeBuildResult& volumeBuildResult = volumeResult.value();
    if (volumeBuildResult.hasWarnings())
    {
        loadingDialog.close();
        if (!m_warningDialogService->confirmWarning(
                "MPR Viewer",
                buildVolumeGeometryWarningMessage("MPR Viewer", volumeBuildResult.warnings),
                "Open Anyway",
                "Cancel"))
        {
            return;
        }
    }
    std::shared_ptr<IVolumeData> volume = volumeBuildResult.volume;
    std::vector<DicomWindowPreset> dicomWindowPresets;
    int activeDicomWindowPresetIndex = -1;
    if (const DicomImage* currentImage = m_viewportController->currentImage();
        currentImage && currentImage->metadata())
    {
        dicomWindowPresets = currentImage->metadata()->windowPresets;
        activeDicomWindowPresetIndex = m_viewportController->currentDicomWindowPresetIndex();
    }

    QWidget* viewer = m_advancedViewerLauncher->showMprVolume(
        std::move(volume),
        buildAdvancedViewerTitle("MPR Viewer", *selectedSeries),
        m_viewportController->currentWindowLevel(),
        m_viewportController->currentWindowWidth(),
        std::move(dicomWindowPresets),
        activeDicomWindowPresetIndex,
        this);
    loadingDialog.close();
    if (viewer)
    {
        viewer->raise();
        viewer->activateWindow();
    }
}

void DicomMainWindow::openThreeDViewer()
{
    if (!m_viewportController || !m_advancedSeriesVolumeService || !m_advancedViewerLauncher)
    {
        return;
    }

    const auto selectedSeries = m_viewportController->currentSeries();
    if (!selectedSeries || selectedSeries->images().size() < 2)
    {
        m_warningDialogService->showWarning("3D Viewer", "Select a multi-slice series before opening 3D.");
        return;
    }

    LoadingDialog loadingDialog(this);
    loadingDialog.show("Loading 3D", "Preparing 3D viewer...");

    const ThreeDProfileSelection profileSelection = ThreeDProfileSelector::selectForSeries(*selectedSeries);
    loadingDialog.setMessage("Loading diagnostic series...");
    const auto diagnosticVolumeResult = m_advancedSeriesVolumeService->buildDiagnosticVolume(*selectedSeries);
    if (!diagnosticVolumeResult)
    {
        loadingDialog.close();
        m_warningDialogService->showError(diagnosticVolumeResult.error());
        return;
    }
    const VolumeBuildResult& volumeBuildResult = diagnosticVolumeResult.value();
    if (volumeBuildResult.hasWarnings())
    {
        loadingDialog.close();
        if (!m_warningDialogService->confirmWarning(
                "3D Viewer",
                buildVolumeGeometryWarningMessage("3D Viewer", volumeBuildResult.warnings),
                "Open Anyway",
                "Cancel"))
        {
            return;
        }
    }
    std::shared_ptr<IVolumeData> diagnosticVolume = volumeBuildResult.volume;

    QWidget* viewer = m_advancedViewerLauncher->showThreeDVolume(
        std::move(diagnosticVolume),
        buildAdvancedViewerTitle("3D Viewer", *selectedSeries),
        profileSelection,
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

    const QString normalizedSpeaker = normalizedAiSpeakerName(speaker).trimmed();
    const QString safeSpeaker = QString("%1:").arg(normalizedSpeaker).toHtmlEscaped();
    const QString formattedMessage = formatAiMessageToHtml(message);
    const bool isUser = speaker.trimmed().compare("You", Qt::CaseInsensitive) == 0;
    const QString alignment = isUser ? "right" : "left";
    const QString bubbleColor = isUser ? "#E7F0FF" : "#D9ECFF";
    const QString bubbleHtml =
        QString(
            "<div style='text-align:%1; margin:10px 0 18px 0;'>"
            "<div style='display:inline-block; max-width:78%%; background:%2; color:#183247; "
            "border:1px solid #b8d2ea; border-radius:12px; padding:8px 12px; text-align:left;'>"
            "<div style='font-weight:700; margin-bottom:6px;'>%3</div>"
            "<div>%4</div>"
            "</div>"
            "</div>"
            "<div><br/><br/></div>")
            .arg(alignment, bubbleColor, safeSpeaker, formattedMessage);

    QTextCursor cursor = m_aiChatHistoryEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
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

    if (!m_currentPatientName.isEmpty())
    {
        contextLines << QString("Patient name: %1").arg(m_currentPatientName);
    }
    if (!m_currentPatientDob.isEmpty())
    {
        contextLines << QString("Patient DOB: %1").arg(m_currentPatientDob);
    }
    if (!m_currentDoctorName.isEmpty())
    {
        contextLines << QString("Doctor: %1").arg(m_currentDoctorName);
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

    if (includeCurrentImage && m_view)
    {
        const QByteArray imageBytes = m_view->captureSnapshotPng();
        if (!imageBytes.isEmpty())
        {
            userMessage.imageAttachments.append({QStringLiteral("image/png"), imageBytes});
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

void DicomMainWindow::onClearAiConversationClicked()
{
    if (!m_aiChatHistoryEdit)
    {
        return;
    }

    m_aiChatHistoryEdit->clear();
    m_aiHistoryShowingStatusMessage = false;
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
                statusMessage += " | Imported into local database";
                refreshHierarchyForGlobalSearch();
                refreshAnnotationReportDock();
            }
            else
            {
                statusMessage += " | Local database save failed";
                m_warningDialogService->showWarning("Database Import", "Failed to save the selected DICOM into the local database.");
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
    QObject* progressTarget = QCoreApplication::instance();
    m_folderImportWatcher->setFuture(QtConcurrent::run([databaseSettings, folderPathCopy, loadingDialog, window, progressTarget]() {
        FolderImportResult result;
        result.folderName = QFileInfo(folderPathCopy).fileName();
        const auto reportProgress = [loadingDialog, window, progressTarget](int value, const QString& message) {
            if (!progressTarget)
            {
                return;
            }

            QMetaObject::invokeMethod(
                progressTarget,
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
        SqliteService databaseService(databaseSettings);
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

void DicomMainWindow::applyViewerToolSelection()
{
    if (!m_view)
    {
        return;
    }

    const std::optional<ViewerToolId> activeTool = selectedViewerTool();
    const bool windowLevelEnabled = activeTool && *activeTool == ViewerToolId::WindowLevel;
    const bool zoomEnabled = activeTool && *activeTool == ViewerToolId::Zoom;
    const bool panEnabled = activeTool && *activeTool == ViewerToolId::Pan;
    const bool distanceEnabled = activeTool && *activeTool == ViewerToolId::Distance;
    const bool polylineEnabled = activeTool && *activeTool == ViewerToolId::Polyline;
    const bool angleEnabled = activeTool && *activeTool == ViewerToolId::Angle;
    const bool rectangleRoiEnabled = activeTool && *activeTool == ViewerToolId::RectangleRoi;

    setViewerInteractionMode(windowLevelEnabled, zoomEnabled, panEnabled);
    m_view->setDistanceMeasurementEnabled(distanceEnabled);
    m_view->setPolylineMeasurementEnabled(polylineEnabled);
    m_view->setAngleMeasurementEnabled(angleEnabled);
    m_view->setRectangleRoiMeasurementEnabled(rectangleRoiEnabled);
}

std::optional<ViewerToolId> DicomMainWindow::selectedViewerTool() const
{
    return m_viewerToolPresentation ? m_viewerToolPresentation->activeTool() : std::nullopt;
}

void DicomMainWindow::onWindowLevelPresetSelected()
{
    if (!m_viewportController || !m_windowLevelPresetComboBox)
    {
        return;
    }

    const int selectedValue = m_windowLevelPresetComboBox->currentData().toInt();
    if (selectedValue >= kDicomWindowPresetBase)
    {
        if (!m_viewportController->applyDicomWindowPreset(selectedValue - kDicomWindowPresetBase))
        {
            return;
        }

        applyImageAdjustments();
        return;
    }

    const auto selectedPreset = static_cast<ViewportWindowPreset>(selectedValue);
    if (selectedPreset == ViewportWindowPreset::Custom)
    {
        m_viewportController->resetPreset();
        syncViewerToolbarState();
        return;
    }

    if (!m_viewportController->applyPreset(selectedPreset))
    {
        return;
    }

    applyImageAdjustments();
}

void DicomMainWindow::onWindowLevelDragDelta(int deltaLevel, int deltaWidth)
{
    if (!m_viewportController)
    {
        return;
    }

    const auto state = m_viewportController->windowControlState(false);
    if (!state.hasImage)
    {
        return;
    }

    const int nextLevel = std::clamp(
        m_viewportController->currentWindowLevel() + deltaLevel,
        state.levelMin,
        state.levelMax);
    const int nextWidth = std::clamp(
        m_viewportController->currentWindowWidth() + deltaWidth,
        state.widthMin,
        state.widthMax);

    m_viewportController->setWindowLevel(nextLevel);
    m_viewportController->setWindowWidth(nextWidth);
    m_viewportController->resetPreset();
    applyImageAdjustments();
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
        QString ignoredLoadError;
        m_viewportController->prepareCurrentSeriesImage(true, &ignoredLoadError);
        m_cineTimer->start(m_viewportController->cineIntervalMs());
    }
    else
    {
        m_cineTimer->stop();
    }
}

void DicomMainWindow::advanceCinePlayback()
{
    if (!m_viewportController || !m_viewportController->hasPlayableSeries())
    {
        return;
    }

    onImageSliderValueChanged(m_viewportController->nextCineIndex());
    m_cineTimer->setInterval(m_viewportController->cineIntervalMs());
}

void DicomMainWindow::exportCurrentCine()
{
    if (!m_viewportController || !m_videoExportController || m_videoExportController->isRunning())
    {
        return;
    }

    DicomImage* image = m_viewportController->currentImage();
    if (!image || image->frameCount() <= 1 || image->filePath().trimmed().isEmpty())
    {
        m_warningDialogService->showWarning(
            QStringLiteral("Export XA Cine"),
            QStringLiteral("Select a multi-frame XA cine object before exporting."));
        return;
    }

    if (!image->hasRawPixels() && !m_viewportController->ensureImageLoaded(*image))
    {
        m_warningDialogService->showWarning(
            QStringLiteral("Export XA Cine"),
            QStringLiteral("The selected XA cine source could not be decoded for export."));
        return;
    }
    if (!image->isMonochrome())
    {
        m_warningDialogService->showWarning(
            QStringLiteral("Export XA Cine"),
            QStringLiteral("Phase 1 cine export currently supports monochrome XA images only."));
        return;
    }

    VideoExportTimingSource timingSource = VideoExportTimingSource::Manual;
    double framesPerSecond = 10.0;
    const std::shared_ptr<const DicomInstanceMetadata> metadata = image->metadata();
    if (metadata &&
        !metadata->frameTimeVectorMs.empty() &&
        image->cineFrameIntervalMs() > 0.0)
    {
        timingSource = VideoExportTimingSource::DicomFrameTime;
        double totalFrameTimeMs = 0.0;
        int validFrameTimes = 0;
        for (double frameTimeMs : metadata->frameTimeVectorMs)
        {
            if (frameTimeMs > 0.0)
            {
                totalFrameTimeMs += frameTimeMs;
                ++validFrameTimes;
            }
        }
        const double averageFrameTimeMs = validFrameTimes > 0
            ? totalFrameTimeMs / static_cast<double>(validFrameTimes)
            : image->cineFrameIntervalMs();
        framesPerSecond = 1000.0 / averageFrameTimeMs;
    }
    else if (image->frameTimeMs() > 0.0)
    {
        timingSource = VideoExportTimingSource::DicomFrameTime;
        framesPerSecond = 1000.0 / image->frameTimeMs();
    }
    else if (image->cineRateFps() > 0.0)
    {
        timingSource = VideoExportTimingSource::DicomCineRate;
        framesPerSecond = image->cineRateFps();
    }
    else if (image->cineFrameIntervalMs() > 0.0)
    {
        framesPerSecond = 1000.0 / image->cineFrameIntervalMs();
    }

    VideoExportController::Context context;
    context.sourceFilePath = image->filePath();
    context.suggestedBaseName = QFileInfo(image->filePath()).completeBaseName();
    context.sourceSopInstanceUid = image->sopInstanceUid();
    context.productVersion = QString::fromUtf8(AppVersion::kVersionString);
    context.frameCount = image->frameCount();
    context.currentFrameIndex = image->frameIndex();
    context.frameSize = QSize(image->width(), image->height());
    context.windowLevel = m_viewportController->currentWindowLevel();
    context.windowWidth = m_viewportController->currentWindowWidth();
    context.defaultFramesPerSecond = std::clamp(framesPerSecond, 1.0, 120.0);
    context.timingSource = timingSource;

    if (!m_videoExportController->startExport(context, this))
    {
        return;
    }

    if (m_cineTimer)
    {
        m_cineTimer->stop();
    }
    m_viewportController->setCinePlaying(false);
    if (m_view)
    {
        m_view->setCinePlaying(false);
    }
    updateCineControls();
}

void DicomMainWindow::onLocalSearchTextChanged(const QString& text)
{
    Q_UNUSED(text);
}

void DicomMainWindow::onGlobalSearchTextChanged(const QString& text)
{
    Q_UNUSED(text);
    refreshHierarchyForGlobalSearch();
}

void DicomMainWindow::onTreePatientContextSelected(
    const QString& patientName,
    const QString& patientDob,
    const QString& doctorName,
    const QString& modality,
    const QString& studyDate)
{
    updatePatientInfo(patientName, patientDob, doctorName, modality, studyDate);
}

void DicomMainWindow::onTreeSeriesSelectionRequested(const QString& seriesInstanceUid)
{
    if (!m_databaseService || seriesInstanceUid.isEmpty())
    {
        return;
    }

    loadSeries(m_databaseService->getSeries(seriesInstanceUid));
}

void DicomMainWindow::onTreeFileSelectionRequested(const QString& filePath)
{
    if (filePath.isEmpty())
    {
        return;
    }

    clearCurrentSeries();
    loadAndDisplayImage(filePath);
}

void DicomMainWindow::onSliceAnnotationsChanged(const QList<SliceMeasurementAnnotationRecord>& records)
{
    if (!m_measurementAnnotationStore || m_activeSliceSopInstanceUid.isEmpty())
    {
        return;
    }

    QSet<QString> currentAnnotationIds;
    for (const SliceMeasurementAnnotationRecord& record : records)
    {
        if (record.measurement.id.isEmpty())
        {
            continue;
        }

        currentAnnotationIds.insert(record.measurement.id);
        if (!m_measurementAnnotationStore->upsertSliceAnnotation(record))
        {
            statusBar()->showMessage("Failed to save slice annotation.", 4000);
        }
    }

    const QSet<QString> removedAnnotationIds = m_loadedSliceAnnotationIds - currentAnnotationIds;
    for (const QString& annotationId : removedAnnotationIds)
    {
        if (!m_measurementAnnotationStore->deleteSliceAnnotation(annotationId))
        {
            statusBar()->showMessage("Failed to delete slice annotation.", 4000);
        }
    }

    m_loadedSliceAnnotationIds = currentAnnotationIds;

    const auto currentSeries = m_viewportController ? m_viewportController->currentSeries() : nullptr;
    if (m_treeController && currentSeries)
    {
        m_treeController->refreshSeriesAnnotationSummary(currentSeries->seriesInstanceUid());
    }
    refreshAnnotationReportDock();
}

void DicomMainWindow::onAnnotationReportFilterChanged()
{
    refreshAnnotationReportDock();
}

void DicomMainWindow::onAnnotationReportGoToSliceRequested(
    const QString& seriesInstanceUid,
    const QString& sopInstanceUid,
    int frameIndex,
    const QString& annotationId)
{
    Q_UNUSED(annotationId);
    if (!m_databaseService || seriesInstanceUid.trimmed().isEmpty())
    {
        return;
    }

    const auto series = m_databaseService->getSeries(seriesInstanceUid);
    if (!series || series->images().empty())
    {
        statusBar()->showMessage("Annotation source series could not be loaded.", 4000);
        return;
    }

    int targetIndex = 0;
    for (int index = 0; index < static_cast<int>(series->images().size()); ++index)
    {
        const auto& image = series->images()[static_cast<std::size_t>(index)];
        if (image && image->sopInstanceUid() == sopInstanceUid && image->frameIndex() == std::max(0, frameIndex))
        {
            targetIndex = index;
            break;
        }
    }

    loadSeries(series, targetIndex);
}

void DicomMainWindow::onAnnotationReportMetadataChanged(
    const QString& annotationId,
    const QString& label,
    const QString& bodyRegion,
    const QString& seriesInstanceUid)
{
    if (!m_annotationReportService)
    {
        return;
    }

    if (!m_annotationReportService->updateMetadata(annotationId, label, bodyRegion))
    {
        statusBar()->showMessage("Failed to update annotation details.", 4000);
        return;
    }

    if (m_treeController)
    {
        m_treeController->refreshSeriesAnnotationSummary(seriesInstanceUid);
    }
    refreshAnnotationReportDock();
}

void DicomMainWindow::onAnnotationReportDeleteRequested(
    const QString& annotationId,
    const QString& seriesInstanceUid,
    const QString& sopInstanceUid,
    int frameIndex)
{
    if (!m_annotationReportService || annotationId.trimmed().isEmpty())
    {
        return;
    }

    if (!m_annotationReportService->deleteAnnotation(annotationId))
    {
        statusBar()->showMessage("Failed to delete annotation.", 4000);
        return;
    }

    if (m_treeController)
    {
        m_treeController->refreshSeriesAnnotationSummary(seriesInstanceUid);
    }

    if (sopInstanceUid == m_activeSliceSopInstanceUid && std::max(0, frameIndex) == m_activeSliceFrameIndex)
    {
        loadCurrentSliceAnnotations();
        return;
    }

    refreshAnnotationReportDock();
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
                                    ? QString("Failed to initialize the local database for folder import.")
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
                                    ? QString("The folder was scanned, but no patient hierarchy could be saved into the local database.")
                                    : result.errorMessage;
        m_warningDialogService->showWarning("Database Import", message);
    }

    refreshHierarchyForGlobalSearch();
    refreshAnnotationReportDock();
    statusBar()->showMessage(
        QString("Imported folder %1 | Patients saved: %2").arg(result.folderName).arg(result.importedPatientCount),
        6000);
}

void DicomMainWindow::refreshHierarchyForGlobalSearch()
{
    if (!m_treeController)
    {
        return;
    }

    m_treeController->refreshHierarchy();
}

void DicomMainWindow::refreshAnnotationReportDock()
{
    if (!m_annotationReportPanel || !m_annotationReportService)
    {
        return;
    }

    const AnnotationCurrentSliceContext context = currentAnnotationSliceContext();
    m_annotationReportPanel->setCurrentSliceContext(context);
    m_annotationReportPanel->setCurrentSliceRows(
        context.hasSlice
            ? m_annotationReportService->loadCurrentSliceRows(
                context.seriesInstanceUid,
                context.sopInstanceUid,
                context.frameIndex)
            : AnnotationReportRows{});

    AnnotationReportFilter filter = m_annotationReportPanel->currentFilter();
    m_annotationReportPanel->setSliceGroups(m_annotationReportService->loadSliceGroups(filter));
}

AnnotationCurrentSliceContext DicomMainWindow::currentAnnotationSliceContext() const
{
    AnnotationCurrentSliceContext context;
    if (!m_viewportController)
    {
        return context;
    }

    const auto currentSeries = m_viewportController->currentSeries();
    const DicomImage* currentImage = m_viewportController->currentImage();
    if (!currentSeries || !currentImage)
    {
        return context;
    }

    context.hasSlice = true;
    context.seriesInstanceUid = currentSeries->seriesInstanceUid();
    context.sopInstanceUid = currentImage->sopInstanceUid();
    context.frameIndex = currentImage->frameIndex();
    context.patientName = m_currentPatientName;
    context.patientAge = m_currentPatientAge;
    context.patientDob = m_currentPatientDob;
    context.doctorName = m_currentDoctorName;
    context.studyDate = m_currentStudyDate;
    context.seriesDescription = currentSeries->seriesDescription();
    context.modality = currentSeries->modality();
    context.instanceNumber = currentImage->instanceNumber();
    context.sliceIndex = m_viewportController->currentImageIndex();
    context.sliceCount = static_cast<int>(currentSeries->images().size());
    return context;
}

void DicomMainWindow::setupAnnotationReportDock()
{
    m_annotationReportDock = new QDockWidget("Annotations", this);
    m_annotationReportDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_annotationReportDock->setFeatures(
        QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);

    m_annotationReportPanel = new AnnotationReportDock(m_annotationReportDock);
    m_annotationReportDock->setWidget(m_annotationReportPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_annotationReportDock);
    m_annotationReportDock->setMinimumHeight(260);
}

void DicomMainWindow::setViewerInteractionMode(bool windowLevelEnabled, bool zoomEnabled, bool panEnabled)
{
    if (!m_view)
    {
        return;
    }

    m_view->setWindowLevelInteractionEnabled(windowLevelEnabled);
    m_view->setZoomInteractionEnabled(zoomEnabled);
    m_view->setPanInteractionEnabled(panEnabled);
    if (windowLevelEnabled || zoomEnabled || panEnabled)
    {
        m_view->setDistanceMeasurementEnabled(false);
        m_view->setPolylineMeasurementEnabled(false);
        m_view->setAngleMeasurementEnabled(false);
        m_view->setRectangleRoiMeasurementEnabled(false);
    }
}

void DicomMainWindow::syncViewerToolbarState()
{
    if (!m_windowLevelToolAction || !m_zoomToolAction || !m_panToolAction || !m_distanceMeasurementToolAction ||
        !m_polylineMeasurementToolAction || !m_angleMeasurementToolAction || !m_rectangleRoiMeasurementToolAction ||
        !m_windowLevelPresetComboBox || !m_view)
    {
        return;
    }

    const bool hasImage = m_viewportController && m_viewportController->currentImage();
    if (!hasImage)
    {
        if (m_viewerToolPresentation)
        {
            m_viewerToolPresentation->setActiveTool(std::nullopt);
        }
        setViewerInteractionMode(false, false, false);
        m_view->setDistanceMeasurementEnabled(false);
        m_view->setPolylineMeasurementEnabled(false);
        m_view->setAngleMeasurementEnabled(false);
        m_view->setRectangleRoiMeasurementEnabled(false);
    }
    if (m_viewerToolPresentation)
    {
        m_viewerToolPresentation->setToolEnabled(ViewerToolId::WindowLevel, hasImage);
        m_viewerToolPresentation->setToolEnabled(ViewerToolId::Zoom, hasImage);
        m_viewerToolPresentation->setToolEnabled(ViewerToolId::Pan, hasImage);
        m_viewerToolPresentation->setToolEnabled(ViewerToolId::Distance, hasImage);
        m_viewerToolPresentation->setToolEnabled(ViewerToolId::Polyline, hasImage);
        m_viewerToolPresentation->setToolEnabled(ViewerToolId::Angle, hasImage);
        m_viewerToolPresentation->setToolEnabled(ViewerToolId::RectangleRoi, hasImage);
    }
    m_windowLevelPresetComboBox->setEnabled(hasImage);

    rebuildWindowLevelPresetComboBox();
    int presetValue = static_cast<int>(hasImage ? m_viewportController->currentPreset() : ViewportWindowPreset::Custom);
    if (hasImage && m_viewportController->currentDicomWindowPresetIndex() >= 0)
    {
        presetValue = kDicomWindowPresetBase + m_viewportController->currentDicomWindowPresetIndex();
    }
    const int comboIndex = std::max(0, m_windowLevelPresetComboBox->findData(presetValue));
    m_windowLevelPresetComboBox->blockSignals(true);
    m_windowLevelPresetComboBox->setCurrentIndex(comboIndex);
    m_windowLevelPresetComboBox->blockSignals(false);
}

void DicomMainWindow::rebuildWindowLevelPresetComboBox()
{
    if (!m_windowLevelPresetComboBox)
    {
        return;
    }

    const int currentValue = m_windowLevelPresetComboBox->currentData().toInt();
    m_windowLevelPresetComboBox->blockSignals(true);
    m_windowLevelPresetComboBox->clear();
    m_windowLevelPresetComboBox->addItem("Custom", static_cast<int>(ViewportWindowPreset::Custom));

    if (m_viewportController)
    {
        const int dicomPresetCount = m_viewportController->dicomWindowPresetCount();
        for (int index = 0; index < dicomPresetCount; ++index)
        {
            const QString label = m_viewportController->dicomWindowPresetLabel(index);
            m_windowLevelPresetComboBox->addItem(
                label.isEmpty() ? QString("DICOM %1").arg(index + 1) : label,
                kDicomWindowPresetBase + index);
        }

        if (dicomPresetCount > 0)
        {
            m_windowLevelPresetComboBox->insertSeparator(m_windowLevelPresetComboBox->count());
        }
    }

    for (const auto& mapping : kBuiltInViewportPresetMappings)
    {
        const auto preset = windowLevelPreset(mapping.builtInPreset);
        m_windowLevelPresetComboBox->addItem(
            windowLevelPresetLabel(preset),
            static_cast<int>(mapping.viewportPreset));
    }

    const int restoredIndex = m_windowLevelPresetComboBox->findData(currentValue);
    if (restoredIndex >= 0)
    {
        m_windowLevelPresetComboBox->setCurrentIndex(restoredIndex);
    }
    m_windowLevelPresetComboBox->blockSignals(false);
}

void DicomMainWindow::applyImageAdjustments()
{
    if (!m_viewportController)
    {
        m_view->clearImage();
        syncViewerToolbarState();
        return;
    }

    const auto windowState = m_viewportController->windowControlState(false);
    const DicomImage* currentImage = m_viewportController->currentImage();
    if (!windowState.hasImage || !currentImage)
    {
        m_view->clearImage();
        syncViewerToolbarState();
        return;
    }

    m_view->setWindowLevelWidth(windowState.level, windowState.width);
    if (currentImage->hasRawPixels())
    {
        displayImageInViewer(*currentImage, windowState.level, windowState.width);
        syncViewerToolbarState();
        return;
    }

    auto diagnosticImageModel = m_viewportController->renderCurrentDiagnosticImage();
    if (!diagnosticImageModel)
    {
        m_view->clearImage();
        syncViewerToolbarState();
        return;
    }
    displayImageInViewer(diagnosticImageModel);
    syncViewerToolbarState();
}
