#pragma once

#include "VTK/MPR/MprTypes.h"

#include <QMainWindow>

#include <memory>

class QAction;
class QComboBox;
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
        QWidget* parent = nullptr);
    ~VtkMprViewerWindow() override;

protected:
    void changeEvent(QEvent* event) override;

private:
    void applyWindowPreset(int index);
    void syncPresetSelection(int level, int width);
    void setupToolbar();
    void setExclusiveToolAction(QAction* activeAction, MprToolType toolType);

private:
    VtkMprView* m_view{nullptr};
    QComboBox* m_presetComboBox{nullptr};
    QAction* m_windowLevelAction{nullptr};
    QAction* m_zoomAction{nullptr};
};
