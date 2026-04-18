#pragma once

#include <QAction>
#include <QComboBox>
#include <QCheckBox>
#include <QDockWidget>
#include <QFutureWatcher>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QModelIndex>
#include <QPlainTextEdit>
#include <QPixmap>
#include <QPushButton>
#include <QSplitter>
#include <QSlider>
#include <QTimer>
#include <QTextEdit>
#include <memory>

#include "DicomGraphicsView.h"
#include "AdvancedViewer/IAdvancedViewerLauncher.h"
#include "AI/AiChatTypes.h"
#include "DicomRenderService.h"
#include "MeasurementController.h"
#include "DicomViewportController.h"
#include "ui_DicomMainWindow.h"

class FileHandling;
class DatabaseService;
class DicomImage;
class IAppConfigService;
class IAiAssistantService;
class IWarningDialogService;
class Patient;
class Series;
class Study;
class VolumeBuilder;
class WindowingAnalysisService;
class LoadingDialog;
class QStandardItem;
class QStandardItemModel;
class QTreeView;
class WarningDialogService;

class DicomMainWindow : public QMainWindow
{
    Q_OBJECT

    Q_DISABLE_COPY_MOVE(DicomMainWindow)

public:
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
    void setupMenuBar();
    void setupConnections();
    void refreshHierarchyTree();
    void addPatientToTree(const std::shared_ptr<Patient>& patient);
    void addStudyToTree(QStandardItem* patientItem, const std::shared_ptr<Patient>& patient, const std::shared_ptr<Study>& study);
    void addSeriesToTree(QStandardItem* studyItem, const std::shared_ptr<Patient>& patient, const std::shared_ptr<Study>& study, const std::shared_ptr<Series>& series);
    void setupImageControlsDock();
    void setupAiDock();
    void clearCurrentSeries();
    void updatePatientInfoPanel(QStandardItem* item);
    void updatePreviewPane(const QPixmap& pixmap);
    void updateSeriesPreview(const std::shared_ptr<Series>& series);
    void applyTreeFilter(const QString& filterText);
    bool updateItemVisibility(QStandardItem* item, const QString& filterText);
    void refreshHierarchyForGlobalSearch();
    void updateCineControls();
    void updateImageControlsState(bool resetWindowState = false);
    void applyImageAdjustments();
    void displayCurrentSlice();
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

private slots:
    void openImage();
    void openFolder();
    void onHierarchyItemActivated(const QModelIndex& index);
    void onHierarchyItemExpanded(const QModelIndex& index);
    void onFolderImportFinished();
    void onImageSliderValueChanged(int value);
    void onSliceWheelRequested(int stepCount);
    void onCineToggled(bool checked);
    void advanceCinePlayback();
    void onLocalSearchTextChanged(const QString& text);
    void onGlobalSearchTextChanged(const QString& text);
    void onToolChanged(DicomGraphicsView::ToolMode toolMode);
    void onWindowLevelChanged(int value);
    void onWindowWidthChanged(int value);
    void onPresetChanged(int index);
    void onAutoWindowPresetChanged(int index);
    void onDistanceMeasurementRequested(const QPoint& startPixel, const QPoint& endPixel);
    void onPixelProbeRequested(const QPoint& pixelPos);
    void onAngleMeasurementRequested(const QPoint& startPixel, const QPoint& vertexPixel, const QPoint& endPixel);
    void onAskAiClicked();
    void onAiRequestFinished();
    void onClearAiConversationClicked();

private:
    Ui::DicomMainWindow* m_ui{nullptr};
    DicomGraphicsView* m_view{nullptr};
    QTreeView* m_treeView{nullptr};
    QSplitter* m_leftPanelSplitter{nullptr};
    QStandardItemModel* m_treeModel{nullptr};
    QDockWidget* m_imageControlsDock{nullptr};
    QDockWidget* m_aiDock{nullptr};
    QLineEdit* m_searchLineEdit{nullptr};
    QLineEdit* m_globalSearchLineEdit{nullptr};
    QComboBox* m_presetComboBox{nullptr};
    QComboBox* m_toolComboBox{nullptr};
    QComboBox* m_autoWindowPresetComboBox{nullptr};
    QComboBox* m_aiModelComboBox{nullptr};
    QComboBox* m_aiReasoningComboBox{nullptr};
    QPushButton* m_openMprButton{nullptr};
    QPushButton* m_aiAskButton{nullptr};
    QPushButton* m_aiClearButton{nullptr};
    QCheckBox* m_aiIncludeImageCheckBox{nullptr};
    QTextEdit* m_aiChatHistoryEdit{nullptr};
    QPlainTextEdit* m_aiPromptEdit{nullptr};
    QLabel* m_patientNameValueLabel{nullptr};
    QLabel* m_patientDobValueLabel{nullptr};
    QLabel* m_patientAgeValueLabel{nullptr};
    QLabel* m_doctorValueLabel{nullptr};
    QLabel* m_modalityValueLabel{nullptr};
    QLabel* m_studyDateValueLabel{nullptr};
    QLabel* m_windowLevelValueLabel{nullptr};
    QLabel* m_windowWidthValueLabel{nullptr};
    QLabel* m_previewTitleLabel{nullptr};
    QLabel* m_previewImageLabel{nullptr};
    QSlider* m_windowLevelSlider{nullptr};
    QSlider* m_windowWidthSlider{nullptr};
    QAction* m_openFileAction{nullptr};
    QAction* m_openFolderAction{nullptr};
    QAction* m_openMprAction{nullptr};
    QAction* m_openThreeDAction{nullptr};
    QAction* m_aiPreferencesAction{nullptr};
    std::unique_ptr<IAppConfigService> m_appConfigService;
    std::unique_ptr<FileHandling> m_gdcmHandler;
    std::unique_ptr<IAdvancedViewerLauncher> m_advancedViewerLauncher;
    std::unique_ptr<VolumeBuilder> m_volumeBuilder;
    std::unique_ptr<WindowingAnalysisService> m_windowingAnalysisService;
    std::unique_ptr<IAiAssistantService> m_aiAssistantService;
    DicomRenderService m_renderService;
    std::unique_ptr<DicomViewportController> m_viewportController;
    std::unique_ptr<DatabaseService> m_databaseService;
    std::unique_ptr<IWarningDialogService> m_warningDialogService;
    QTimer* m_cineTimer{nullptr};
    QFutureWatcher<AiChatResponse>* m_aiResponseWatcher{nullptr};
    QFutureWatcher<FolderImportResult>* m_folderImportWatcher{nullptr};
    LoadingDialog* m_folderImportLoadingDialog{nullptr};
    MeasurementController m_measurementController;
    bool m_aiHistoryShowingStatusMessage{false};
};
