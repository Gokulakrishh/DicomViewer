#pragma once

#include <QAction>
#include <QComboBox>
#include <QCheckBox>
#include <QDockWidget>
#include <QFutureWatcher>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPixmap>
#include <QPushButton>
#include <QSplitter>
#include <QSlider>
#include <QTimer>
#include <QTextEdit>
#include <QToolBar>
#include <memory>

#include "AdvancedViewer/IAdvancedViewerLauncher.h"
#include "AI/AiChatTypes.h"
#include "AnnotationReportDock.h"
#include "DicomTreeController.h"
#include "DicomTreePanel.h"
#include "DicomViewportController.h"
#include "Services/MeasurementAnnotationStore.h"
#include "ViewerTools/ViewerToolPresentation.h"
#include "VTK/MainView/VtkDiagnosticSliceView.h"
#include "ui_DicomMainWindow.h"

class FileHandling;
class AuditService;
class DatabaseService;
class DicomImage;
class IAppConfigService;
class IAiAssistantService;
class IWarningDialogService;
class Patient;
class Series;
class SeriesPreviewService;
class Study;
class AdvancedSeriesVolumeService;
class AnnotationReportService;
class LoadingDialog;
class VideoExportController;
class WarningDialogService;

/**
 * @brief Main application window for local DICOM viewing workflows.
 *
 * Responsibilities:
 * - Coordinate study browsing, slice display, WL/WW, viewer tools, annotations,
 *   and advanced viewer launch.
 * - Keep heavy DICOM pixel loading in service/controller layers instead of the
 *   UI tree model.
 * - Route annotation persistence and reporting through service abstractions.
 *
 * Assumptions:
 * - The application is local-first and uses source DICOM files as canonical
 *   image data.
 * - Viewer features are being developed toward medical-device traceability, but
 *   UI coordination code is not itself a regulatory control.
 */
class DicomMainWindow : public QMainWindow
{
    Q_OBJECT

    Q_DISABLE_COPY_MOVE(DicomMainWindow)

public:
    /**
     * @brief Creates the main DICOM viewer window.
     * @param appConfigService Application configuration service.
     * @param advancedViewerLauncher Launcher for MPR and 3D windows.
     * @param warningDialogService Warning dialog presenter.
     * @param parent Optional Qt parent.
     */
    explicit DicomMainWindow(
        std::unique_ptr<IAppConfigService> appConfigService,
        std::unique_ptr<IAdvancedViewerLauncher> advancedViewerLauncher,
        std::unique_ptr<IWarningDialogService> warningDialogService,
        QWidget* parent = nullptr);

    ~DicomMainWindow();

private:
    struct FolderImportResult
    {
        QString folderName;
        int importedPatientCount{0};
        bool foundImportableDicom{false};
        bool initializeSucceeded{false};
        bool hadSaveFailure{false};
        QString errorMessage;
    };

    void setUiComponents();
    void setupStudyBrowserDock();
    void setupViewerSurface();
    void setupViewerToolbar();
    void setupCoreServices();
    void setupAsyncInfrastructure();
    void setupAnnotationReportDock();
    void initializeDatabaseAndTree();
    void setupMenuBar();
    void setupConnections();
    void clearCurrentSeries();
    void updatePatientInfo(
        const QString& patientName,
        const QString& patientDob,
        const QString& doctorName,
        const QString& modality,
        const QString& studyDate);
    void refreshHierarchyForGlobalSearch();
    void refreshAnnotationReportDock();
    [[nodiscard]] AnnotationCurrentSliceContext currentAnnotationSliceContext() const;
    void updateCineControls();
    void applyImageAdjustments();
    void displayImageInViewer(const DicomImage& image, int windowLevel, int windowWidth);
    void displayImageInViewer(const std::shared_ptr<DicomImage>& image);
    void displayCurrentSlice();
    void loadCurrentSliceAnnotations();
    void loadSeries(const std::shared_ptr<Series>& series, int initialIndex = 0);
    void loadAndDisplayImage(const QString& filePath);
    void openMprViewer();
    void openThreeDViewer();
    void openAiPreferences();
    void appendAiMessage(const QString& speaker, const QString& message);
    QString normalizedAiSpeakerName(const QString& speaker) const;
    QString buildAiContextPrompt() const;
    AiChatRequest buildAiChatRequest(const QString& userPrompt, bool includeCurrentImage) const;
    void rebuildAiAssistantService();
    void refreshAiDockState();
    void syncViewerToolbarState();
    void rebuildWindowLevelPresetComboBox();
    void applyViewerToolSelection();
    [[nodiscard]] std::optional<ViewerToolId> selectedViewerTool() const;
    void setViewerInteractionMode(bool windowLevelEnabled, bool zoomEnabled, bool panEnabled = false);

private slots:
    void openImage();
    void openFolder();
    void onFolderImportFinished();
    void onImageSliderValueChanged(int value);
    void onSliceWheelRequested(int stepCount);
    void onCineToggled(bool checked);
    void advanceCinePlayback();
    void exportCurrentCine();
    void onLocalSearchTextChanged(const QString& text);
    void onGlobalSearchTextChanged(const QString& text);
    void onAskAiClicked();
    void onAiRequestFinished();
    void onClearAiConversationClicked();
    void onWindowLevelPresetSelected();
    void onWindowLevelDragDelta(int deltaLevel, int deltaWidth);
    void onTreePatientContextSelected(
        const QString& patientName,
        const QString& patientDob,
        const QString& doctorName,
        const QString& modality,
        const QString& studyDate);
    void onTreeSeriesSelectionRequested(const QString& seriesInstanceUid);
    void onTreeFileSelectionRequested(const QString& filePath);
    void onSliceAnnotationsChanged(const QList<SliceMeasurementAnnotationRecord>& records);
    void onAnnotationReportFilterChanged();
    void onAnnotationReportGoToSliceRequested(
        const QString& seriesInstanceUid,
        const QString& sopInstanceUid,
        int frameIndex,
        const QString& annotationId);
    void onAnnotationReportMetadataChanged(
        const QString& annotationId,
        const QString& label,
        const QString& bodyRegion,
        const QString& seriesInstanceUid);
    void onAnnotationReportDeleteRequested(
        const QString& annotationId,
        const QString& seriesInstanceUid,
        const QString& sopInstanceUid,
        int frameIndex);

private:
    Ui::DicomMainWindow* m_ui{nullptr};
    VtkDiagnosticSliceView* m_view{nullptr};
    QDockWidget* m_studyBrowserDock{nullptr};
    DicomTreePanel* m_treePanel{nullptr};
    DicomTreeController* m_treeController{nullptr};
    QDockWidget* m_annotationReportDock{nullptr};
    AnnotationReportDock* m_annotationReportPanel{nullptr};
    QDockWidget* m_aiDock{nullptr};
    QComboBox* m_aiModelComboBox{nullptr};
    QComboBox* m_aiReasoningComboBox{nullptr};
    QPushButton* m_aiAskButton{nullptr};
    QPushButton* m_aiClearButton{nullptr};
    QCheckBox* m_aiIncludeImageCheckBox{nullptr};
    QTextEdit* m_aiChatHistoryEdit{nullptr};
    QPlainTextEdit* m_aiPromptEdit{nullptr};
    QAction* m_openFileAction{nullptr};
    QAction* m_openFolderAction{nullptr};
    QAction* m_openMprAction{nullptr};
    QAction* m_openThreeDAction{nullptr};
    QAction* m_windowLevelToolAction{nullptr};
    QAction* m_zoomToolAction{nullptr};
    QAction* m_panToolAction{nullptr};
    QAction* m_distanceMeasurementToolAction{nullptr};
    QAction* m_polylineMeasurementToolAction{nullptr};
    QAction* m_angleMeasurementToolAction{nullptr};
    QAction* m_rectangleRoiMeasurementToolAction{nullptr};
    QAction* m_exportCineAction{nullptr};
    QToolBar* m_viewerToolBar{nullptr};
    std::unique_ptr<ViewerToolPresentation> m_viewerToolPresentation;
    QComboBox* m_windowLevelPresetComboBox{nullptr};
    std::unique_ptr<IAppConfigService> m_appConfigService;
    std::unique_ptr<AuditService> m_auditService;
    std::unique_ptr<FileHandling> m_gdcmHandler;
    std::unique_ptr<IAdvancedViewerLauncher> m_advancedViewerLauncher;
    std::unique_ptr<AdvancedSeriesVolumeService> m_advancedSeriesVolumeService;
    std::unique_ptr<IAiAssistantService> m_aiAssistantService;
    std::unique_ptr<DicomViewportController> m_viewportController;
    std::unique_ptr<DatabaseService> m_databaseService;
    std::unique_ptr<MeasurementAnnotationStore> m_measurementAnnotationStore;
    std::unique_ptr<AnnotationReportService> m_annotationReportService;
    std::unique_ptr<SeriesPreviewService> m_seriesPreviewService;
    std::unique_ptr<IWarningDialogService> m_warningDialogService;
    VideoExportController* m_videoExportController{nullptr};
    QTimer* m_cineTimer{nullptr};
    QFutureWatcher<AiChatResponse>* m_aiResponseWatcher{nullptr};
    QFutureWatcher<FolderImportResult>* m_folderImportWatcher{nullptr};
    LoadingDialog* m_folderImportLoadingDialog{nullptr};
    bool m_resetViewerFitOnNextImage{true};
    QString m_currentPatientName;
    QString m_currentPatientDob;
    QString m_currentPatientAge;
    QString m_currentDoctorName;
    QString m_currentModality;
    QString m_currentStudyDate;
    QString m_activeSliceSopInstanceUid;
    int m_activeSliceFrameIndex{0};
    QSet<QString> m_loadedSliceAnnotationIds;
    bool m_aiHistoryShowingStatusMessage{false};
};
