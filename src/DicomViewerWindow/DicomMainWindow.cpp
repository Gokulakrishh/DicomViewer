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
#include <QRegularExpression>
#include <QSet>
#include <QSlider>
#include <QStatusBar>
#include <QStringList>
#include <QTextCursor>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent/QtConcurrent>
#include <algorithm>
#include <cmath>

#include "AI/GeminiAiAssistantService.h"
#include "AI/IAiAssistantService.h"
#include "AI/QtHttpAiServerClient.h"
#include "Utilities/AiApiKeyDialog.h"
#include "Utilities/AiMessageFormatter.h"
#include "Utilities/DiagnosticImageRenderer.h"
#include "Utilities/IAppConfigService.h"
#include "Utilities/MemoryManagementDebug.h"
#include "Utilities/LoadingDialog.h"
#include "Utilities/IWarningDialogService.h"
#include "Database/PostgreService.h"
#include "FileHandling/GDCMFileHandling.h"
#include "Model/MedicalImage.h"
#include "Services/AdvancedSeriesVolumeService.h"
#include "Services/ThreeDProfiles/ThreeDProfileSelection.h"

namespace
{
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
    setupLeftPanel();
    setupViewerSurface();
    setupLegacyViewerControls();
    setupCoreServices();
    setupAiDock();
    setupAsyncInfrastructure();
    initializeDatabaseAndTree();
    setupConnections();
}

void DicomMainWindow::setupLeftPanel()
{
    m_treePanel = new DicomTreePanel(this);
    m_treeController = new DicomTreeController(this);
    m_ui->horizontalLayout->replaceWidget(m_ui->leftPanelHost, m_treePanel);
    m_ui->leftPanelHost->deleteLater();
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

void DicomMainWindow::setupLegacyViewerControls()
{
    m_ui->contrastVerticalSlider->hide();
    m_ui->imageVerticalSlider->hide();
    m_ui->cineCheckBox->hide();
    m_ui->cineLabel->hide();
    m_ui->viewerControlsSeparator->hide();
}

void DicomMainWindow::setupCoreServices()
{
    m_cineTimer = new QTimer(this);
    m_cineTimer->setInterval(100);

    m_gdcmHandler = std::make_unique<GDCMFileHandling>();
    m_advancedSeriesVolumeService = std::make_unique<AdvancedSeriesVolumeService>(
        *m_gdcmHandler,
        m_appConfigService->loadVolumeValidationSettings());
    m_viewportController = std::make_unique<DicomViewportController>(
        m_gdcmHandler.get(),
        this);
    rebuildAiAssistantService();
    m_databaseService = std::make_unique<PostgreService>(m_appConfigService->loadDatabaseSettings());
    if (m_treeController)
    {
        m_treeController->setDatabaseService(m_databaseService.get());
    }
}

void DicomMainWindow::setupAsyncInfrastructure()
{
    m_aiResponseWatcher = new QFutureWatcher<AiChatResponse>(this);
    m_folderImportWatcher = new QFutureWatcher<FolderImportResult>(this);
}

void DicomMainWindow::initializeDatabaseAndTree()
{
    const bool databaseInitialized = m_databaseService->initialize();
    if (!databaseInitialized)
    {
        const QString warningMessage =
            m_databaseService->lastErrorText() + " Config file: " + m_appConfigService->configFilePath();
        statusBar()->showMessage(warningMessage, 10000);
        m_warningDialogService->showWarning("Database Configuration", warningMessage);
    }

    refreshHierarchyForGlobalSearch();
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
    m_openThreeDAction = new QAction("3D", this);
    m_ui->viewMenu->addAction(m_openThreeDAction);

    connect(m_openFileAction, &QAction::triggered, this, &DicomMainWindow::openImage);
    connect(m_openFolderAction, &QAction::triggered, this, &DicomMainWindow::openFolder);
    connect(m_aiPreferencesAction, &QAction::triggered, this, &DicomMainWindow::openAiPreferences);
    connect(m_openMprAction, &QAction::triggered, this, &DicomMainWindow::openMprViewer);
    connect(m_openThreeDAction, &QAction::triggered, this, &DicomMainWindow::openThreeDViewer);
}

void DicomMainWindow::setupViewerToolbar()
{
    const auto presetValue = [](ViewportWindowPreset preset) {
        return static_cast<int>(preset);
    };

    m_viewerToolBar = addToolBar("Viewer");
    m_viewerToolBar->setMovable(false);

    m_windowLevelToolAction = new QAction("WL/WW", this);
    m_windowLevelToolAction->setCheckable(true);
    m_viewerToolBar->addAction(m_windowLevelToolAction);
    connect(m_windowLevelToolAction, &QAction::toggled, this, &DicomMainWindow::onWindowLevelToolToggled);

    m_zoomToolAction = new QAction("Zoom", this);
    m_zoomToolAction->setCheckable(true);
    m_viewerToolBar->addAction(m_zoomToolAction);
    connect(m_zoomToolAction, &QAction::toggled, this, &DicomMainWindow::onZoomToolToggled);

    m_viewerToolBar->addSeparator();

    auto* presetLabel = new QLabel("Preset:", this);
    m_viewerToolBar->addWidget(presetLabel);

    m_windowLevelPresetComboBox = new QComboBox(this);
    m_windowLevelPresetComboBox->addItem("Custom", presetValue(ViewportWindowPreset::Custom));
    m_windowLevelPresetComboBox->addItem("Brain", presetValue(ViewportWindowPreset::Brain));
    m_windowLevelPresetComboBox->addItem("Soft Tissue", presetValue(ViewportWindowPreset::SoftTissue));
    m_windowLevelPresetComboBox->addItem("Bone", presetValue(ViewportWindowPreset::Bone));
    m_windowLevelPresetComboBox->addItem("Lung", presetValue(ViewportWindowPreset::Lung));
    m_viewerToolBar->addWidget(m_windowLevelPresetComboBox);
    connect(m_windowLevelPresetComboBox, &QComboBox::currentIndexChanged, this, &DicomMainWindow::onWindowLevelPresetSelected);
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
    if (m_view)
    {
        m_view->setPatientInfoText({}, {}, {}, {}, {}, {});
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

    if (m_view)
    {
        m_view->setPatientInfoText(
            m_currentPatientName,
            m_currentPatientAge,
            m_currentPatientDob,
            m_currentDoctorName,
            m_currentModality,
            m_currentStudyDate);
    }
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
        const QString warningMessage = "Failed to load image: " + QFileInfo(failedFilePath).fileName();
        statusBar()->showMessage(warningMessage, 4000);
        m_warningDialogService->showWarning("Image Loading", warningMessage);
        m_view->clearImage();
        return;
    }

    if (!cinePlaying)
    {
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
    MemoryManagementDebug::logMainViewerSnapshot(m_viewportController.get(), m_view, "displayCurrentSlice");
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
    if (m_treePanel)
    {
        m_treePanel->updateSeriesPreview(series);
    }
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
        m_resetViewerFitOnNextImage = true;
        m_view->setSliceNavigationState(0, 1);
        m_view->setCineAvailable(false);
        m_view->setCinePlaying(false);
        if (m_treePanel)
        {
            m_treePanel->updatePreviewPane(createDicomPreviewPixmap(*singleImage));
        }
        applyImageAdjustments();
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
    std::shared_ptr<IVolumeData> volume = volumeResult.value();

    QWidget* viewer = m_advancedViewerLauncher->showMprVolume(
        std::move(volume),
        buildAdvancedViewerTitle("MPR Viewer", *selectedSeries),
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
    std::shared_ptr<IVolumeData> diagnosticVolume = diagnosticVolumeResult.value();

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
                statusMessage += " | Imported into PostgreSQL";
                refreshHierarchyForGlobalSearch();
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

void DicomMainWindow::onWindowLevelToolToggled(bool checked)
{
    if (checked && m_zoomToolAction && m_zoomToolAction->isChecked())
    {
        m_zoomToolAction->blockSignals(true);
        m_zoomToolAction->setChecked(false);
        m_zoomToolAction->blockSignals(false);
    }
    setViewerInteractionMode(checked, false);
}

void DicomMainWindow::onZoomToolToggled(bool checked)
{
    if (checked && m_windowLevelToolAction && m_windowLevelToolAction->isChecked())
    {
        m_windowLevelToolAction->blockSignals(true);
        m_windowLevelToolAction->setChecked(false);
        m_windowLevelToolAction->blockSignals(false);
    }
    setViewerInteractionMode(false, checked);
}

void DicomMainWindow::onWindowLevelPresetSelected()
{
    if (!m_viewportController || !m_windowLevelPresetComboBox)
    {
        return;
    }

    const auto selectedPreset = static_cast<ViewportWindowPreset>(m_windowLevelPresetComboBox->currentData().toInt());
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
        m_cineTimer->start();
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

    refreshHierarchyForGlobalSearch();
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
    m_aiClearButton = new QPushButton("Clear", dockContentWidget);

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
    auto* actionLayout = new QHBoxLayout();
    actionLayout->setContentsMargins(0, 0, 0, 0);
    actionLayout->setSpacing(8);
    actionLayout->addWidget(m_aiClearButton);
    actionLayout->addWidget(m_aiAskButton, 1);
    dockLayout->addLayout(actionLayout);

    m_aiDock->setWidget(dockContentWidget);
    addDockWidget(Qt::RightDockWidgetArea, m_aiDock);
    m_aiDock->setMinimumHeight(260);
    refreshAiDockState();
}

void DicomMainWindow::setViewerInteractionMode(bool windowLevelEnabled, bool zoomEnabled)
{
    if (!m_view)
    {
        return;
    }

    m_view->setWindowLevelInteractionEnabled(windowLevelEnabled);
    m_view->setZoomInteractionEnabled(zoomEnabled);
}

void DicomMainWindow::syncViewerToolbarState()
{
    if (!m_windowLevelToolAction || !m_zoomToolAction || !m_windowLevelPresetComboBox || !m_view)
    {
        return;
    }

    const bool hasImage = m_viewportController && m_viewportController->currentImage();
    if (!hasImage)
    {
        m_windowLevelToolAction->blockSignals(true);
        m_windowLevelToolAction->setChecked(false);
        m_windowLevelToolAction->blockSignals(false);
        m_zoomToolAction->blockSignals(true);
        m_zoomToolAction->setChecked(false);
        m_zoomToolAction->blockSignals(false);
        setViewerInteractionMode(false, false);
    }
    m_windowLevelToolAction->setEnabled(hasImage);
    m_zoomToolAction->setEnabled(hasImage);
    m_windowLevelPresetComboBox->setEnabled(hasImage);

    const int presetValue = static_cast<int>(hasImage ? m_viewportController->currentPreset() : ViewportWindowPreset::Custom);
    const int comboIndex = std::max(0, m_windowLevelPresetComboBox->findData(presetValue));
    m_windowLevelPresetComboBox->blockSignals(true);
    m_windowLevelPresetComboBox->setCurrentIndex(comboIndex);
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

    const int windowLevel = m_viewportController->currentWindowLevel();
    const int windowWidth = m_viewportController->currentWindowWidth();
    m_view->setWindowLevelWidth(windowLevel, windowWidth);

    const DicomImage* currentImage = m_viewportController->currentImage();
    if (!currentImage)
    {
        m_view->clearImage();
        syncViewerToolbarState();
        return;
    }

    if (currentImage->hasRawPixels())
    {
        displayImageInViewer(*currentImage, windowLevel, windowWidth);
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
