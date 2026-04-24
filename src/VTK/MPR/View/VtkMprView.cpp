#include "VTK/MPR/View/VtkMprView.h"

#include "Model/IVolumeData.h"
#include "VTK/Adapters/VtkVolumeAdapter.h"
#include "VTK/MPR/Adapters/MprToolAdapter.h"
#include "VTK/MPR/Adapters/VtkMprSceneAdapter.h"
#include "VTK/MPR/Sync/MprSynchronizationService.h"
#include "VTK/MPR/View/VtkSliceMprPaneView.h"
#include "VTK/MPR/View/VtkThreeDReferencePaneView.h"

#include <QMouseEvent>
#include <QEvent>
#include <QGridLayout>
#include <QString>
#include <QSignalBlocker>
#include <QSlider>
#include <QVTKOpenGLNativeWidget.h>
#include <QWheelEvent>
#include <algorithm>
#include <utility>

VtkMprView::VtkMprView(
    std::shared_ptr<IVolumeData> volume,
    int initialWindowLevel,
    int initialWindowWidth,
    QWidget* parent)
    : QWidget(parent),
      m_volume(std::move(volume)),
      m_imageData(VtkVolumeAdapter::createImageData(*m_volume)),
      m_scene(this),
      m_controller(m_scene),
      m_sceneAdapter(std::make_unique<VtkMprSceneAdapter>()),
      m_toolAdapter(std::make_unique<MprToolAdapter>(m_controller, *m_sceneAdapter)),
      m_toolController(m_scene, m_controller, *m_toolAdapter),
      m_interactionRouter(m_toolController, m_controller)
{
    Q_UNUSED(initialWindowLevel);

    setupUi();
    m_scene.setWindowLevelWidth(initialWindowLevel, initialWindowWidth);
    configureScene();
    configureBindings();
    m_scene.setCursorPosition(m_sceneAdapter->centeredCursorPositionWorld());
    configureSliders();
}

VtkMprView::~VtkMprView() = default;

void VtkMprView::setActiveTool(MprToolType toolType)
{
    m_toolController.setActiveTool(toolType);
}

void VtkMprView::setWindowLevelWidth(int level, int width)
{
    m_controller.setWindowLevelWidth(level, width);
}

void VtkMprView::setContextText(const QString& text)
{
    m_contextText = text.trimmed();
    updatePaneStatusText();
}

int VtkMprView::currentWindowLevel() const
{
    return m_scene.windowLevel();
}

int VtkMprView::currentWindowWidth() const
{
    return m_scene.windowWidth();
}

MprToolType VtkMprView::activeTool() const
{
    return m_scene.activeTool();
}

void VtkMprView::setupUi()
{
    auto* layout = new QGridLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    m_axialPane = std::make_unique<VtkSliceMprPaneView>("Axial", MprSlicePlane::Axial, this);
    m_coronalPane = std::make_unique<VtkSliceMprPaneView>("Coronal", MprSlicePlane::Coronal, this);
    m_sagittalPane = std::make_unique<VtkSliceMprPaneView>("Sagittal", MprSlicePlane::Sagittal, this);
    m_referencePane = std::make_unique<VtkThreeDReferencePaneView>("3D Reference", this);

    m_axialPane->renderWidget()->installEventFilter(this);
    m_coronalPane->renderWidget()->installEventFilter(this);
    m_sagittalPane->renderWidget()->installEventFilter(this);

    layout->addWidget(m_axialPane->widget(), 0, 0);
    layout->addWidget(m_coronalPane->widget(), 0, 1);
    layout->addWidget(m_sagittalPane->widget(), 1, 0);
    layout->addWidget(m_referencePane->widget(), 1, 1);
}

bool VtkMprView::eventFilter(QObject* watched, QEvent* event)
{
    if (watched != m_axialPane->renderWidget() &&
        watched != m_coronalPane->renderWidget() &&
        watched != m_sagittalPane->renderWidget())
    {
        return QWidget::eventFilter(watched, event);
    }

    const auto plane = planeForRenderWidget(watched);

    switch (event->type())
    {
    case QEvent::Resize:
        refreshOverlayState();
        return QWidget::eventFilter(watched, event);
    case QEvent::Wheel:
        handleWheelEvent(plane, static_cast<QWheelEvent*>(event));
        event->accept();
        return true;
    default:
        break;
    }

    switch (m_scene.activeTool())
    {
    case MprToolType::None:
    case MprToolType::Crosshair:
    case MprToolType::WindowLevel:
    case MprToolType::Zoom:
        switch (event->type())
        {
        case QEvent::MouseButtonPress:
        {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton)
            {
                m_lastInteractionPosition = normalizedPositionForEvent(watched, mouseEvent->position());
                m_interactionRouter.beginInteraction(
                    plane,
                    mouseEvent->button(),
                    m_lastInteractionPosition);
                event->accept();
                return true;
            }
            break;
        }
        case QEvent::MouseMove:
        {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->buttons().testFlag(Qt::LeftButton))
            {
                const QPointF currentPosition = normalizedPositionForEvent(watched, mouseEvent->position());
                m_interactionRouter.updateInteraction(
                    plane,
                    currentPosition,
                    currentPosition - m_lastInteractionPosition);
                m_lastInteractionPosition = currentPosition;
                updatePaneStatusText();
                event->accept();
                return true;
            }
            break;
        }
        case QEvent::MouseButtonRelease:
        {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton)
            {
                m_interactionRouter.endInteraction(
                    plane,
                    mouseEvent->button(),
                    normalizedPositionForEvent(watched, mouseEvent->position()));
                event->accept();
                return true;
            }
            break;
        }
        case QEvent::MouseButtonDblClick:
        case QEvent::ContextMenu:
            event->accept();
            return true;
        default:
            break;
        }
        return QWidget::eventFilter(watched, event);
    case MprToolType::Slice:
        switch (event->type())
        {
        default:
            return QWidget::eventFilter(watched, event);
        }
    }

    return QWidget::eventFilter(watched, event);
}

void VtkMprView::handleWheelEvent(MprSlicePlane plane, QWheelEvent* event)
{
    const QPoint numDegrees = event->angleDelta() / 8;
    const QPoint numSteps = numDegrees / 15;
    int steps = numSteps.y();
    if (steps == 0)
    {
        steps = (event->pixelDelta().y() > 0) ? 1 : (event->pixelDelta().y() < 0 ? -1 : 0);
    }

    if (steps != 0)
    {
        m_interactionRouter.scrollSlices(plane, steps);
        updatePaneStatusText();
    }
}

void VtkMprView::refreshOverlayState()
{
    updatePaneStatusText();

    const MprCursorPositionWorld position = m_scene.cursorPosition();
    m_axialPane->setCrosshairPosition(
        MprSynchronizationService::normalizedPositionForPlane(
            *m_imageData,
            position,
            MprSlicePlane::Axial));
    m_coronalPane->setCrosshairPosition(
        MprSynchronizationService::normalizedPositionForPlane(
            *m_imageData,
            position,
            MprSlicePlane::Coronal));
    m_sagittalPane->setCrosshairPosition(
        MprSynchronizationService::normalizedPositionForPlane(
            *m_imageData,
            position,
            MprSlicePlane::Sagittal));
}

void VtkMprView::updatePaneStatusText()
{
    const QString context = displayContextText();
    const int wl = m_scene.windowLevel();
    const int ww = m_scene.windowWidth();

    m_axialPane->setContextText(context);
    m_coronalPane->setContextText(context);
    m_sagittalPane->setContextText(context);

    m_axialPane->setSliceText(QString("Slice: %1").arg(m_sceneAdapter->currentSlice(MprSlicePlane::Axial)));
    m_coronalPane->setSliceText(QString("Slice: %1").arg(m_sceneAdapter->currentSlice(MprSlicePlane::Coronal)));
    m_sagittalPane->setSliceText(QString("Slice: %1").arg(m_sceneAdapter->currentSlice(MprSlicePlane::Sagittal)));

    m_axialPane->setWindowLevelText(
        QString("WL/WW: %1 / %2")
            .arg(wl)
            .arg(ww));
    m_coronalPane->setWindowLevelText(
        QString("WL/WW: %1 / %2")
            .arg(wl)
            .arg(ww));
    m_sagittalPane->setWindowLevelText(
        QString("WL/WW: %1 / %2")
            .arg(wl)
            .arg(ww));
    m_axialPane->setZoomText(QString("Zoom: %1%").arg(m_sceneAdapter->zoomPercent(MprSlicePlane::Axial)));
    m_coronalPane->setZoomText(QString("Zoom: %1%").arg(m_sceneAdapter->zoomPercent(MprSlicePlane::Coronal)));
    m_sagittalPane->setZoomText(QString("Zoom: %1%").arg(m_sceneAdapter->zoomPercent(MprSlicePlane::Sagittal)));
}

QString VtkMprView::displayContextText() const
{
    if (!m_contextText.isEmpty())
    {
        return QString("Patient/Study: %1").arg(m_contextText);
    }
    return QStringLiteral("Patient/Study: -");
}

MprSlicePlane VtkMprView::planeForRenderWidget(QObject* watched) const
{
    if (watched == m_axialPane->renderWidget())
    {
        return MprSlicePlane::Axial;
    }
    if (watched == m_coronalPane->renderWidget())
    {
        return MprSlicePlane::Coronal;
    }
    return MprSlicePlane::Sagittal;
}

QPointF VtkMprView::normalizedPositionForEvent(QObject* watched, const QPointF& position) const
{
    auto* widget = qobject_cast<QWidget*>(watched);
    if (!widget || widget->width() <= 0 || widget->height() <= 0)
    {
        return {0.5, 0.5};
    }

    return {
        std::clamp(position.x() / static_cast<double>(widget->width()), 0.0, 1.0),
        std::clamp(position.y() / static_cast<double>(widget->height()), 0.0, 1.0)};
}

void VtkMprView::configureScene()
{
    m_sceneAdapter->attachPane(MprSlicePlane::Axial, *m_axialPane->renderWidget());
    m_sceneAdapter->attachPane(MprSlicePlane::Coronal, *m_coronalPane->renderWidget());
    m_sceneAdapter->attachPane(MprSlicePlane::Sagittal, *m_sagittalPane->renderWidget());
    m_sceneAdapter->attachReferencePane(*m_referencePane->renderWidget());
    m_sceneAdapter->initialize(*m_imageData, m_scene.windowLevel(), m_scene.windowWidth());
    m_controller.setImageData(m_sceneAdapter->imageData());

    m_axialPane->sliceSlider()->setRange(
        m_sceneAdapter->sliceMin(MprSlicePlane::Axial),
        m_sceneAdapter->sliceMax(MprSlicePlane::Axial));
    m_coronalPane->sliceSlider()->setRange(
        m_sceneAdapter->sliceMin(MprSlicePlane::Coronal),
        m_sceneAdapter->sliceMax(MprSlicePlane::Coronal));
    m_sagittalPane->sliceSlider()->setRange(
        m_sceneAdapter->sliceMin(MprSlicePlane::Sagittal),
        m_sceneAdapter->sliceMax(MprSlicePlane::Sagittal));
    refreshOverlayState();
}

void VtkMprView::configureBindings()
{
    connect(&m_scene, &MprScene::cursorPositionChanged, this, [this](const MprCursorPositionWorld& position) {
        m_sceneAdapter->applyCursorPositionWorld(position);

        const QSignalBlocker axialBlocker(m_axialPane->sliceSlider());
        const QSignalBlocker coronalBlocker(m_coronalPane->sliceSlider());
        const QSignalBlocker sagittalBlocker(m_sagittalPane->sliceSlider());

        m_axialPane->sliceSlider()->setValue(m_sceneAdapter->currentSlice(MprSlicePlane::Axial));
        m_coronalPane->sliceSlider()->setValue(m_sceneAdapter->currentSlice(MprSlicePlane::Coronal));
        m_sagittalPane->sliceSlider()->setValue(m_sceneAdapter->currentSlice(MprSlicePlane::Sagittal));

        m_axialPane->setCrosshairPosition(
            MprSynchronizationService::normalizedPositionForPlane(
                *m_imageData,
                position,
                MprSlicePlane::Axial));
        m_coronalPane->setCrosshairPosition(
            MprSynchronizationService::normalizedPositionForPlane(
                *m_imageData,
                position,
                MprSlicePlane::Coronal));
        m_sagittalPane->setCrosshairPosition(
            MprSynchronizationService::normalizedPositionForPlane(
                *m_imageData,
                position,
                MprSlicePlane::Sagittal));
        updatePaneStatusText();
        m_sceneAdapter->renderAll();
    });

    connect(&m_scene, &MprScene::windowLevelWidthChanged, this, [this](int level, int width) {
        m_sceneAdapter->applyWindowLevelWidth(level, width);
        updatePaneStatusText();
        emit windowLevelWidthChanged(level, width);
        m_sceneAdapter->renderAll();
    });

    connect(&m_scene, &MprScene::activeToolChanged, this, [this](MprToolType toolType) {
        Q_UNUSED(toolType);
    });

    m_axialPane->setCrosshairVisible(true);
    m_coronalPane->setCrosshairVisible(true);
    m_sagittalPane->setCrosshairVisible(true);
    refreshOverlayState();
}

void VtkMprView::configureSliders()
{
    connect(m_axialPane->sliceSlider(), &QSlider::valueChanged, this, [this](int value) {
        m_controller.setSlice(MprSlicePlane::Axial, value);
    });
    connect(m_coronalPane->sliceSlider(), &QSlider::valueChanged, this, [this](int value) {
        m_controller.setSlice(MprSlicePlane::Coronal, value);
    });
    connect(m_sagittalPane->sliceSlider(), &QSlider::valueChanged, this, [this](int value) {
        m_controller.setSlice(MprSlicePlane::Sagittal, value);
    });
}
