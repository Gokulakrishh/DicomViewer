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
class IVolumeData;
class VtkMprView;

class VtkMprViewerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit VtkMprViewerWindow(
        std::shared_ptr<IVolumeData> volume,
        int initialWindowLevel,
        int initialWindowWidth,
        std::vector<DicomWindowPreset> dicomWindowPresets = {},
        int activeDicomWindowPresetIndex = -1,
        QWidget* parent = nullptr);
    ~VtkMprViewerWindow() override;

protected:
    void changeEvent(QEvent* event) override;

private:
    void applyWindowPreset(int index);
    void applyToolSelection();
    void syncPresetSelection(int level, int width);
    void setupToolbar();
    int comboValueForBuiltInPresetIndex(int index) const;
    int comboValueForDicomPresetIndex(int index) const;

private:
    VtkMprView* m_view{nullptr};
    QComboBox* m_presetComboBox{nullptr};
    std::unique_ptr<ViewerToolPresentation> m_toolPresentation;
    std::vector<DicomWindowPreset> m_dicomWindowPresets;
    int m_activeDicomWindowPresetIndex{-1};
};
