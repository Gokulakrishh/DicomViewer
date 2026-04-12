#pragma once

#include <QAction>
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QModelIndex>
#include <QPixmap>
#include <QSplitter>
#include <QSlider>
#include <QTimer>
#include <memory>

#include "DicomGraphicsView.h"
#include "DicomRenderService.h"
#include "MeasurementController.h"
#include "DicomViewportController.h"
#include "ui_DicomMainWindow.h"

class FileHandling;
class DatabaseService;
class DicomImage;
class Patient;
class Series;
class QStandardItem;
class QStandardItemModel;
class QTreeView;
class WarningDialogService;

class DicomMainWindow : public QMainWindow
{
    Q_OBJECT

    Q_DISABLE_COPY_MOVE(DicomMainWindow)

public:
    explicit DicomMainWindow(QWidget* parent = nullptr);
    ~DicomMainWindow();

private:
    void setUiComponents();
    void setupMenuBar();
    void setupConnections();
    void refreshHierarchyTree();
    void addPatientToTree(const std::shared_ptr<Patient>& patient);
    void setupImageControlsDock();
    void clearCurrentSeries();
    void updatePatientInfoPanel(QStandardItem* item);
    void updatePreviewPane(const QPixmap& pixmap);
    void updateSeriesPreview(const std::shared_ptr<Series>& series);
    void applyTreeFilter(const QString& filterText);
    bool updateItemVisibility(QStandardItem* item, const QString& filterText);
    void updateCineControls();
    void updateImageControlsState(bool resetWindowState = false);
    void applyImageAdjustments();
    void displayCurrentSlice();
    void loadSeries(const std::shared_ptr<Series>& series, int initialIndex = 0);
    void loadAndDisplayImage(const QString& filePath);

private slots:
    void openImage();
    void openFolder();
    void onHierarchyItemActivated(const QModelIndex& index);
    void onImageSliderValueChanged(int value);
    void onSliceWheelRequested(int stepCount);
    void onCineToggled(bool checked);
    void advanceCinePlayback();
    void onSearchTextChanged(const QString& text);
    void onToolChanged(int index);
    void onWindowLevelChanged(int value);
    void onWindowWidthChanged(int value);
    void onPresetChanged(int index);
    void onDistanceMeasurementRequested(const QPoint& startPixel, const QPoint& endPixel);
    void onPixelProbeRequested(const QPoint& pixelPos);
    void onAngleMeasurementRequested(const QPoint& startPixel, const QPoint& vertexPixel, const QPoint& endPixel);

private:
    Ui::DicomMainWindow* m_ui{nullptr};
    DicomGraphicsView* m_view{nullptr};
    QTreeView* m_treeView{nullptr};
    QSplitter* m_leftPanelSplitter{nullptr};
    QStandardItemModel* m_treeModel{nullptr};
    QDockWidget* m_imageControlsDock{nullptr};
    QLineEdit* m_searchLineEdit{nullptr};
    QComboBox* m_presetComboBox{nullptr};
    QComboBox* m_toolComboBox{nullptr};
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
    std::unique_ptr<FileHandling> m_gdcmHandler;
    DicomRenderService m_renderService;
    std::unique_ptr<DicomViewportController> m_viewportController;
    std::unique_ptr<DatabaseService> m_databaseService;
    std::unique_ptr<WarningDialogService> m_warningDialogService;
    QTimer* m_cineTimer{nullptr};
    MeasurementController m_measurementController;
};
