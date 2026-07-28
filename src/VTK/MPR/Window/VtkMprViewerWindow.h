#pragma once

#include "VTK/MPR/MprTypes.h"
#include "Model/DicomMetadata.h"
#include "ViewerTools/ViewerToolPresentation.h"

#include <QMainWindow>

#include <memory>
#include <vector>

class QAction;
class QComboBox;
class QMenu;
class QDockWidget;
class QDoubleSpinBox;
class QLabel;
class DatabaseService;
class IVolumeData;
class MprMeasurementAnnotationStore;
class VtkMprAnnotationDock;
class VtkMprView;

/**
 * @brief Top-level MPR viewer window.
 *
 * Responsibilities:
 * - Own MPR toolbar/preset presentation.
 * - Host VtkMprView for synchronized orthogonal slice viewing.
 */
class VtkMprViewerWindow : public QMainWindow
{
    Q_OBJECT

public:
    /** @brief Creates the MPR viewer window. */
    explicit VtkMprViewerWindow(
        std::shared_ptr<IVolumeData> volume,
        int initialWindowLevel,
        int initialWindowWidth,
        std::vector<DicomWindowPreset> dicomWindowPresets = {},
        int activeDicomWindowPresetIndex = -1,
        QString seriesInstanceUid = {},
        DatabaseService* databaseService = nullptr,
        QWidget* parent = nullptr);
    ~VtkMprViewerWindow() override;

protected:
    void changeEvent(QEvent* event) override;

private:
    void applyWindowPreset(int index);
    void applySlabControls();
    void applyObliqueControls();
    void applyToolSelection();
    void syncPresetSelection(int level, int width);
    void setupToolbar();
    void setupAnnotationDock();
    int comboValueForBuiltInPresetIndex(int index) const;
    int comboValueForDicomPresetIndex(int index) const;

private:
    VtkMprView* m_view{nullptr};
    QDockWidget* m_annotationDockWidget{nullptr};
    VtkMprAnnotationDock* m_annotationDock{nullptr};
    QComboBox* m_presetComboBox{nullptr};
    QComboBox* m_slabModeComboBox{nullptr};
    QDoubleSpinBox* m_slabThicknessSpinBox{nullptr};
    QComboBox* m_obliquePlaneComboBox{nullptr};
    QDoubleSpinBox* m_obliqueAngleSpinBox{nullptr};
    std::unique_ptr<ViewerToolPresentation> m_toolPresentation;
    std::unique_ptr<MprMeasurementAnnotationStore> m_annotationStore;
    std::vector<DicomWindowPreset> m_dicomWindowPresets;
    int m_activeDicomWindowPresetIndex{-1};
};
