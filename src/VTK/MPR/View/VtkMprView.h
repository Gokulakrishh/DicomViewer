#pragma once

#include "VTK/MPR/Controllers/InteractionRouter.h"
#include "VTK/MPR/Controllers/MprController.h"
#include "VTK/MPR/Controllers/ToolController.h"
#include "VTK/MPR/State/MprScene.h"

#include <QWidget>
#include <vtkSmartPointer.h>

#include <memory>

class IVolumeData;
class MprToolAdapter;
class VtkMprSceneAdapter;
class VtkSliceMprPaneView;
class VtkThreeDReferencePaneView;
class vtkImageData;

class VtkMprView : public QWidget
{
    Q_OBJECT

public:
    explicit VtkMprView(
        std::shared_ptr<IVolumeData> volume,
        int initialWindowLevel,
        int initialWindowWidth,
        QWidget* parent = nullptr);
    ~VtkMprView() override;

    void setActiveTool(MprToolType toolType);
    void setContextText(const QString& text);
    void setWindowLevelWidth(int level, int width);
    [[nodiscard]] int currentWindowLevel() const;
    [[nodiscard]] int currentWindowWidth() const;
    [[nodiscard]] MprToolType activeTool() const;

signals:
    void windowLevelWidthChanged(int level, int width);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void refreshOverlayState();
    void updatePaneStatusText();
    [[nodiscard]] QString displayContextText() const;
    [[nodiscard]] MprSlicePlane planeForRenderWidget(QObject* watched) const;
    [[nodiscard]] QPointF normalizedPositionForEvent(QObject* watched, const QPointF& position) const;
    void handleWheelEvent(MprSlicePlane plane, QWheelEvent* event);
    void setupUi();
    void configureScene();
    void configureBindings();
    void configureSliders();

private:
    std::shared_ptr<IVolumeData> m_volume;
    vtkSmartPointer<vtkImageData> m_imageData;
    MprScene m_scene;
    MprController m_controller;
    std::unique_ptr<VtkMprSceneAdapter> m_sceneAdapter;
    std::unique_ptr<MprToolAdapter> m_toolAdapter;
    ToolController m_toolController;
    InteractionRouter m_interactionRouter;
    std::unique_ptr<VtkSliceMprPaneView> m_axialPane;
    std::unique_ptr<VtkSliceMprPaneView> m_coronalPane;
    std::unique_ptr<VtkSliceMprPaneView> m_sagittalPane;
    std::unique_ptr<VtkThreeDReferencePaneView> m_referencePane;
    QPointF m_lastInteractionPosition{0.5, 0.5};
    QString m_contextText;
};
