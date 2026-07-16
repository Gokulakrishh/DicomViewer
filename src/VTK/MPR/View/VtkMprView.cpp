#include "VTK/MPR/View/VtkMprView.h"

#include "Model/IVolumeData.h"
#include "VTK/Adapters/VtkVolumeAdapter.h"
#include "VTK/MPR/Adapters/MprToolAdapter.h"
#include "VTK/MPR/Adapters/VtkMprSceneAdapter.h"
#include "VTK/MPR/MprMeasurementScalarSource.h"
#include "VTK/MPR/View/VtkSliceMprPaneView.h"
#include "VTK/MPR/View/VtkThreeDReferencePaneView.h"
#include "Services/MprMeasurementAnnotationStore.h"
#include "ViewerTools/Measurements/MeasurementAnalyticsService.h"
#include "ViewerTools/ViewerInputEvent.h"

#include <QDebug>
#include <QDateTime>
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

namespace
{
QString axisLabelForPlane(MprSlicePlane plane)
{
    switch (plane)
    {
    case MprSlicePlane::Axial:
        return QStringLiteral("Z");
    case MprSlicePlane::Coronal:
        return QStringLiteral("Y");
    case MprSlicePlane::Sagittal:
        return QStringLiteral("X");
    }

    return QStringLiteral("-");
}

QString planeTypeToString(MprSlicePlane plane)
{
    switch (plane)
    {
    case MprSlicePlane::Axial:
        return QStringLiteral("Axial");
    case MprSlicePlane::Coronal:
        return QStringLiteral("Coronal");
    case MprSlicePlane::Sagittal:
        return QStringLiteral("Sagittal");
    }

    return QStringLiteral("MPR");
}

MprSlicePlane planeTypeFromString(const QString& value)
{
    const QString normalizedValue = value.trimmed().toLower();
    if (normalizedValue == "coronal")
    {
        return MprSlicePlane::Coronal;
    }
    if (normalizedValue == "sagittal")
    {
        return MprSlicePlane::Sagittal;
    }
    return MprSlicePlane::Axial;
}

double axisPositionForPlane(MprSlicePlane plane, const MprCursorPositionWorld& position)
{
    switch (plane)
    {
    case MprSlicePlane::Axial:
        return position.z;
    case MprSlicePlane::Coronal:
        return position.y;
    case MprSlicePlane::Sagittal:
        return position.x;
    }

    return 0.0;
}

double axisPositionForPlane(MprSlicePlane plane, const MeasurementPoint& point)
{
    switch (plane)
    {
    case MprSlicePlane::Axial:
        return point.z;
    case MprSlicePlane::Coronal:
        return point.y;
    case MprSlicePlane::Sagittal:
        return point.x;
    }

    return 0.0;
}

QString slicePositionText(MprSlicePlane plane, int slice, const MprCursorPositionWorld& position)
{
    return QString("Slice: %1 | %2: %3 mm")
        .arg(slice)
        .arg(axisLabelForPlane(plane))
        .arg(axisPositionForPlane(plane, position), 0, 'f', 1);
}
}

VtkMprView::VtkMprView(
    std::shared_ptr<IVolumeData> volume,
    int initialWindowLevel,
    int initialWindowWidth,
    QString seriesInstanceUid,
    MprMeasurementAnnotationStore* annotationStore,
    QWidget* parent)
    : QWidget(parent),
      m_volume(std::move(volume)),
      m_imageData(VtkVolumeAdapter::createImageData(*m_volume)),
      m_scene(this),
      m_controller(m_scene),
      m_sceneAdapter(std::make_unique<VtkMprSceneAdapter>()),
      m_toolAdapter(std::make_unique<MprToolAdapter>(m_controller, *m_sceneAdapter)),
      m_toolController(m_scene, m_controller, m_measurementService, *this, *m_toolAdapter),
      m_interactionRouter(m_toolController, m_controller),
      m_seriesInstanceUid(std::move(seriesInstanceUid)),
      m_annotationStore(annotationStore)
{
    Q_UNUSED(initialWindowLevel);

    setupUi();
    m_scene.setWindowLevelWidth(initialWindowLevel, initialWindowWidth);
    configureScene();
    configureBindings();
    m_scene.setCursorPosition(m_sceneAdapter->centeredCursorPositionWorld());
    configureSliders();
    loadPersistedMprAnnotations();
    publishMprAnnotationRecords();
}

VtkMprView::~VtkMprView() = default;

void VtkMprView::setActiveTool(MprToolType toolType)
{
    if (m_scene.activeTool() != toolType)
    {
        m_measurementService.cancelActiveMeasurement();
        refreshMeasurementOverlays();
    }
    m_toolController.setActiveTool(toolType);
    updateCursorState();
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

QList<MprMeasurementAnnotationRecord> VtkMprView::mprAnnotationRecords() const
{
    return currentMprAnnotationRecords();
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
    m_referencePane->renderWidget()->installEventFilter(this);

    layout->addWidget(m_axialPane->widget(), 0, 0);
    layout->addWidget(m_coronalPane->widget(), 0, 1);
    layout->addWidget(m_sagittalPane->widget(), 1, 0);
    layout->addWidget(m_referencePane->widget(), 1, 1);
}

bool VtkMprView::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_referencePane->renderWidget())
    {
        switch (event->type())
        {
        case QEvent::MouseButtonDblClick:
            m_sceneAdapter->resetReferenceCamera();
            event->accept();
            return true;
        case QEvent::Wheel:
        case QEvent::NativeGesture:
        case QEvent::Gesture:
            event->accept();
            return true;
        default:
            break;
        }
        return QWidget::eventFilter(watched, event);
    }

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
    case MprToolType::Pan:
        if (handlePanEvent(watched, event, plane))
        {
            return true;
        }
        return QWidget::eventFilter(watched, event);
    case MprToolType::DistanceMeasurement:
    case MprToolType::PolylineMeasurement:
    case MprToolType::AngleMeasurement:
    case MprToolType::RectangleRoiMeasurement:
        if (handleMeasurementEvent(watched, event, plane))
        {
            return true;
        }
        return QWidget::eventFilter(watched, event);
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
                refreshOverlayState();
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
        normalizedCrosshairPositionForPane(*m_axialPane, MprSlicePlane::Axial, position));
    m_coronalPane->setCrosshairPosition(
        normalizedCrosshairPositionForPane(*m_coronalPane, MprSlicePlane::Coronal, position));
    m_sagittalPane->setCrosshairPosition(
        normalizedCrosshairPositionForPane(*m_sagittalPane, MprSlicePlane::Sagittal, position));
    refreshMeasurementOverlays();
}

void VtkMprView::refreshMeasurementOverlays()
{
    m_axialPane->setMeasurements(displayMeasurementsForPane(*m_axialPane, MprSlicePlane::Axial));
    m_coronalPane->setMeasurements(displayMeasurementsForPane(*m_coronalPane, MprSlicePlane::Coronal));
    m_sagittalPane->setMeasurements(displayMeasurementsForPane(*m_sagittalPane, MprSlicePlane::Sagittal));
}

void VtkMprView::updatePaneStatusText()
{
    const QString context = displayContextText();
    const int wl = m_scene.windowLevel();
    const int ww = m_scene.windowWidth();
    const MprCursorPositionWorld cursorPosition = m_scene.cursorPosition();

    m_axialPane->setContextText(context);
    m_coronalPane->setContextText(context);
    m_sagittalPane->setContextText(context);

    m_axialPane->setSliceText(slicePositionText(
        MprSlicePlane::Axial,
        m_sceneAdapter->currentSlice(MprSlicePlane::Axial),
        cursorPosition));
    m_coronalPane->setSliceText(slicePositionText(
        MprSlicePlane::Coronal,
        m_sceneAdapter->currentSlice(MprSlicePlane::Coronal),
        cursorPosition));
    m_sagittalPane->setSliceText(slicePositionText(
        MprSlicePlane::Sagittal,
        m_sceneAdapter->currentSlice(MprSlicePlane::Sagittal),
        cursorPosition));

    m_axialPane->setWindowLevelText(
        QString("WL: %1 WW: %2")
            .arg(wl)
            .arg(ww));
    m_coronalPane->setWindowLevelText(
        QString("WL: %1 WW: %2")
            .arg(wl)
            .arg(ww));
    m_sagittalPane->setWindowLevelText(
        QString("WL: %1 WW: %2")
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

    if (m_scene.activeTool() == MprToolType::None ||
        m_scene.activeTool() == MprToolType::Crosshair)
    {
        return m_sceneAdapter->normalizedImagePositionFromDisplayPosition(
            planeForRenderWidget(watched),
            position,
            widget->size(),
            m_scene.cursorPosition());
    }

    return {
        std::clamp(position.x() / static_cast<double>(widget->width()), 0.0, 1.0),
        std::clamp(position.y() / static_cast<double>(widget->height()), 0.0, 1.0)};
}

QPointF VtkMprView::normalizedCrosshairPositionForPane(
    VtkSliceMprPaneView& pane,
    MprSlicePlane plane,
    const MprCursorPositionWorld& position) const
{
    return m_sceneAdapter->normalizedDisplayPositionForCursorWorld(
        plane,
        position,
        pane.renderWidget()->size());
}

bool VtkMprView::handleMeasurementEvent(QObject* watched, QEvent* event, MprSlicePlane plane)
{
    auto* widget = qobject_cast<QWidget*>(watched);
    if (!widget)
    {
        return false;
    }

    ViewerInputEvent inputEvent;
    inputEvent.plane = plane;
    inputEvent.widgetSize = widget->size();
    m_lastMeasurementInputPlane = plane;

    switch (event->type())
    {
    case QEvent::MouseMove:
    {
        const auto* mouseEvent = static_cast<QMouseEvent*>(event);
        inputEvent.eventType = ViewerInputEvent::EventType::MouseMove;
        inputEvent.phase = ViewerInputEvent::Phase::Update;
        inputEvent.button = Qt::NoButton;
        inputEvent.displayPosition = mouseEvent->position();
        m_toolController.updateInteraction(inputEvent);
        event->accept();
        return true;
    }
    case QEvent::MouseButtonPress:
    {
        const auto* mouseEvent = static_cast<QMouseEvent*>(event);
        inputEvent.eventType = ViewerInputEvent::EventType::MousePress;
        inputEvent.phase = ViewerInputEvent::Phase::Begin;
        inputEvent.button = mouseEvent->button();
        inputEvent.displayPosition = mouseEvent->position();
        m_toolController.beginInteraction(inputEvent);
        event->accept();
        return true;
    }
    case QEvent::MouseButtonRelease:
    {
        const auto* mouseEvent = static_cast<QMouseEvent*>(event);
        inputEvent.eventType = ViewerInputEvent::EventType::MouseRelease;
        inputEvent.phase = ViewerInputEvent::Phase::End;
        inputEvent.button = mouseEvent->button();
        inputEvent.displayPosition = mouseEvent->position();
        m_toolController.endInteraction(inputEvent);
        event->accept();
        return true;
    }
    case QEvent::MouseButtonDblClick:
    {
        const auto* mouseEvent = static_cast<QMouseEvent*>(event);
        inputEvent.eventType = ViewerInputEvent::EventType::MouseDoubleClick;
        inputEvent.phase = ViewerInputEvent::Phase::Begin;
        inputEvent.button = mouseEvent->button();
        inputEvent.displayPosition = mouseEvent->position();
        m_toolController.beginInteraction(inputEvent);
        event->accept();
        return true;
    }
    default:
        return false;
    }
}

MeasurementPoint VtkMprView::measurementPointForInput(const ViewerInputEvent& event) const
{
    QWidget* widget = nullptr;
    switch (event.plane)
    {
    case MprSlicePlane::Axial:
        widget = m_axialPane ? m_axialPane->renderWidget() : nullptr;
        break;
    case MprSlicePlane::Coronal:
        widget = m_coronalPane ? m_coronalPane->renderWidget() : nullptr;
        break;
    case MprSlicePlane::Sagittal:
        widget = m_sagittalPane ? m_sagittalPane->renderWidget() : nullptr;
        break;
    }

    if (!widget)
    {
        return {};
    }

    return measurementPointForEvent(event.plane, *widget, event.displayPosition);
}

void VtkMprView::onMeasurementToolUpdated()
{
    captureNewMeasurementPlaneContexts();
    persistNewMprMeasurements();
    publishMprAnnotationRecords();
    refreshMeasurementOverlays();
}

void VtkMprView::goToMprAnnotation(const QString& annotationId)
{
    if (annotationId.trimmed().isEmpty())
    {
        return;
    }

    const auto it = std::find_if(
        m_measurementService.measurements().cbegin(),
        m_measurementService.measurements().cend(),
        [&annotationId](const MeasurementAnnotation& measurement) {
            return measurement.id == annotationId;
        });
    if (it == m_measurementService.measurements().cend() || it->points.isEmpty())
    {
        return;
    }

    const MeasurementPoint& point = it->points.first();
    m_scene.setCursorPosition({point.x, point.y, point.z});
}

void VtkMprView::deleteMprAnnotation(const QString& annotationId)
{
    const QString normalizedId = annotationId.trimmed();
    if (normalizedId.isEmpty())
    {
        return;
    }

    if (m_annotationStore && !m_annotationStore->deleteMprAnnotation(normalizedId))
    {
        qWarning() << "Failed to delete MPR annotation" << normalizedId;
        return;
    }

    if (m_measurementService.removeMeasurement(normalizedId))
    {
        m_measurementPlaneById.remove(normalizedId);
        m_annotationRecordById.remove(normalizedId);
        m_persistedMprAnnotationIds.remove(normalizedId);
        refreshMeasurementOverlays();
        publishMprAnnotationRecords();
    }
}

void VtkMprView::updateMprAnnotationMetadata(
    const QString& annotationId,
    const QString& label,
    const QString& bodyRegion,
    const QString& note)
{
    const QString normalizedId = annotationId.trimmed();
    if (normalizedId.isEmpty())
    {
        return;
    }

    if (m_annotationStore &&
        !m_annotationStore->updateMprAnnotationMetadata(normalizedId, label, bodyRegion, note))
    {
        qWarning() << "Failed to update MPR annotation metadata" << normalizedId;
        return;
    }

    MprMeasurementAnnotationRecord record = m_annotationRecordById.value(normalizedId);
    if (record.measurement.id.isEmpty())
    {
        const auto it = std::find_if(
            m_measurementService.measurements().cbegin(),
            m_measurementService.measurements().cend(),
            [&normalizedId](const MeasurementAnnotation& measurement) {
                return measurement.id == normalizedId;
            });
        if (it == m_measurementService.measurements().cend())
        {
            return;
        }
        record = toMprAnnotationRecord(
            *it,
            m_measurementPlaneById.value(normalizedId, m_lastMeasurementInputPlane));
    }

    record.label = label.trimmed();
    record.bodyRegion = bodyRegion.trimmed().isEmpty() ? QStringLiteral("Other") : bodyRegion.trimmed();
    record.note = note.trimmed();
    record.updatedAtUtc = QDateTime::currentDateTimeUtc();
    m_annotationRecordById.insert(normalizedId, record);
    publishMprAnnotationRecords();
}

bool VtkMprView::handlePanEvent(QObject* watched, QEvent* event, MprSlicePlane plane)
{
    auto* widget = qobject_cast<QWidget*>(watched);
    if (!widget)
    {
        return false;
    }

    switch (event->type())
    {
    case QEvent::MouseButtonPress:
    {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() != Qt::LeftButton)
        {
            return false;
        }
        m_lastInteractionPosition = mouseEvent->position();
        m_panDragActive = true;
        updateCursorState();
        m_toolController.beginInteraction(makePanViewerInputEvent(
            ViewerInputEvent::Phase::Begin,
            plane,
            mouseEvent->button(),
            mouseEvent->position(),
            {},
            widget->size()));
        event->accept();
        return true;
    }
    case QEvent::MouseMove:
    {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (!mouseEvent->buttons().testFlag(Qt::LeftButton))
        {
            return false;
        }
        const QPointF currentPosition = mouseEvent->position();
        m_toolController.updateInteraction(makePanViewerInputEvent(
            ViewerInputEvent::Phase::Update,
            plane,
            Qt::NoButton,
            currentPosition,
            currentPosition - m_lastInteractionPosition,
            widget->size()));
        m_lastInteractionPosition = currentPosition;
        refreshOverlayState();
        event->accept();
        return true;
    }
    case QEvent::MouseButtonRelease:
    {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() != Qt::LeftButton)
        {
            return false;
        }
        m_panDragActive = false;
        updateCursorState();
        m_toolController.endInteraction(makePanViewerInputEvent(
            ViewerInputEvent::Phase::End,
            plane,
            mouseEvent->button(),
            mouseEvent->position(),
            {},
            widget->size()));
        event->accept();
        return true;
    }
    default:
        break;
    }

    return false;
}

void VtkMprView::updateCursorState()
{
    const bool isPanTool = m_scene.activeTool() == MprToolType::Pan;
    const QCursor cursor = m_panDragActive ? QCursor(Qt::ClosedHandCursor) : QCursor(Qt::OpenHandCursor);
    auto applyCursor = [&](QWidget* widget) {
        if (!widget)
        {
            return;
        }
        if (isPanTool)
        {
            widget->setCursor(cursor);
        }
        else
        {
            widget->unsetCursor();
        }
    };

    applyCursor(m_axialPane->renderWidget());
    applyCursor(m_coronalPane->renderWidget());
    applyCursor(m_sagittalPane->renderWidget());
}

MeasurementPoint VtkMprView::measurementPointForEvent(
    MprSlicePlane plane,
    QWidget& widget,
    const QPointF& position) const
{
    const MprCursorPositionWorld worldPosition = m_sceneAdapter->worldPositionFromDisplayPosition(
        plane,
        position,
        widget.size(),
        m_scene.cursorPosition());
    return {worldPosition.x, worldPosition.y, worldPosition.z};
}

QString VtkMprView::measurementLabel(
    const MeasurementAnnotation& measurement,
    MprSlicePlane plane) const
{
    switch (measurement.type)
    {
    case MeasurementType::Distance:
    case MeasurementType::Polyline:
        return MeasurementService::formattedLength(measurement.lengthMm);
    case MeasurementType::Angle:
        return MeasurementService::formattedAngle(MeasurementService::angleDegrees(measurement.points));
    case MeasurementType::RectangleRoi:
        return MeasurementAnalyticsService::rectangleRoiLabel(
            measurement,
            MprMeasurementScalarSource(m_volume.get(), *m_sceneAdapter, plane));
    }

    return {};
}

bool VtkMprView::measurementBelongsToCurrentSlice(
    const MeasurementAnnotation& measurement,
    MprSlicePlane plane) const
{
    if (measurement.points.isEmpty())
    {
        return false;
    }

    const double currentPlanePosition = axisPositionForPlane(plane, m_scene.cursorPosition());
    const double tolerance = sliceToleranceMm(plane);
    return std::all_of(
        measurement.points.cbegin(),
        measurement.points.cend(),
        [plane, currentPlanePosition, tolerance](const MeasurementPoint& point) {
            return std::abs(axisPositionForPlane(plane, point) - currentPlanePosition) <= tolerance;
        });
}

double VtkMprView::sliceToleranceMm(MprSlicePlane plane) const
{
    if (!m_volume || !m_volume->geometry().isValid())
    {
        return 0.01;
    }

    const auto& spacing = m_volume->geometry().spacing;
    double axisSpacing = spacing.z;
    switch (plane)
    {
    case MprSlicePlane::Axial:
        axisSpacing = spacing.z;
        break;
    case MprSlicePlane::Coronal:
        axisSpacing = spacing.y;
        break;
    case MprSlicePlane::Sagittal:
        axisSpacing = spacing.x;
        break;
    }

    return std::max(0.01, axisSpacing * 0.5);
}

void VtkMprView::loadPersistedMprAnnotations()
{
    if (!m_annotationStore || m_seriesInstanceUid.trimmed().isEmpty())
    {
        return;
    }

    const QList<MprMeasurementAnnotationRecord> records = m_annotationStore->loadMprAnnotations(m_seriesInstanceUid);
    QVector<MeasurementAnnotation> measurements;
    measurements.reserve(records.size());
    m_measurementPlaneById.clear();
    m_annotationRecordById.clear();
    m_persistedMprAnnotationIds.clear();

    for (const MprMeasurementAnnotationRecord& record : records)
    {
        if (record.measurement.id.isEmpty())
        {
            continue;
        }
        measurements.append(record.measurement);
        m_measurementPlaneById.insert(record.measurement.id, planeTypeFromString(record.planeType));
        m_annotationRecordById.insert(record.measurement.id, record);
        m_persistedMprAnnotationIds.insert(record.measurement.id);
    }

    if (!measurements.isEmpty())
    {
        m_measurementService.setMeasurements(measurements);
        refreshMeasurementOverlays();
    }
}

void VtkMprView::captureNewMeasurementPlaneContexts()
{
    for (const MeasurementAnnotation& measurement : m_measurementService.measurements())
    {
        if (!measurement.id.isEmpty() && !m_measurementPlaneById.contains(measurement.id))
        {
            m_measurementPlaneById.insert(measurement.id, m_lastMeasurementInputPlane);
        }
    }
}

void VtkMprView::persistNewMprMeasurements()
{
    if (!m_annotationStore || m_seriesInstanceUid.trimmed().isEmpty())
    {
        return;
    }

    for (const MeasurementAnnotation& measurement : m_measurementService.measurements())
    {
        if (measurement.id.isEmpty() || m_persistedMprAnnotationIds.contains(measurement.id))
        {
            continue;
        }

        const MprSlicePlane plane = m_measurementPlaneById.value(measurement.id, m_lastMeasurementInputPlane);
        const MprMeasurementAnnotationRecord record = toMprAnnotationRecord(measurement, plane);
        if (m_annotationStore->upsertMprAnnotation(record))
        {
            m_persistedMprAnnotationIds.insert(measurement.id);
            m_annotationRecordById.insert(measurement.id, record);
        }
        else
        {
            qWarning() << "Failed to save MPR annotation" << measurement.id;
        }
    }
}

void VtkMprView::publishMprAnnotationRecords()
{
    emit mprAnnotationsChanged(currentMprAnnotationRecords());
}

QList<MprMeasurementAnnotationRecord> VtkMprView::currentMprAnnotationRecords() const
{
    QList<MprMeasurementAnnotationRecord> records;
    records.reserve(m_measurementService.measurements().size());
    for (const MeasurementAnnotation& measurement : m_measurementService.measurements())
    {
        if (measurement.id.isEmpty())
        {
            continue;
        }
        MprMeasurementAnnotationRecord record = toMprAnnotationRecord(
            measurement,
            m_measurementPlaneById.value(measurement.id, m_lastMeasurementInputPlane));
        if (const auto storedRecord = m_annotationRecordById.constFind(measurement.id);
            storedRecord != m_annotationRecordById.constEnd())
        {
            record.label = storedRecord->label;
            record.bodyRegion = storedRecord->bodyRegion;
            record.note = storedRecord->note;
            record.createdAtUtc = storedRecord->createdAtUtc;
            record.updatedAtUtc = storedRecord->updatedAtUtc;
        }
        records.append(record);
    }
    return records;
}

MprMeasurementAnnotationRecord VtkMprView::toMprAnnotationRecord(
    const MeasurementAnnotation& measurement,
    MprSlicePlane plane) const
{
    MprMeasurementAnnotationRecord record;
    record.seriesInstanceUid = m_seriesInstanceUid;
    record.planeType = planeTypeToString(plane);
    record.measurement = measurement;
    record.label = QString("%1 annotation").arg(record.planeType);
    record.bodyRegion = QStringLiteral("Other");
    record.planePositionMm = measurement.points.isEmpty()
        ? axisPositionForPlane(plane, m_scene.cursorPosition())
        : axisPositionForPlane(plane, measurement.points.first());
    if (measurement.type == MeasurementType::Angle)
    {
        record.angleDegrees = MeasurementService::angleDegrees(measurement.points);
    }
    if (measurement.type == MeasurementType::RectangleRoi)
    {
        record.roiStatistics = MeasurementAnalyticsService::rectangleRoiStatistics(
            measurement,
            MprMeasurementScalarSource(m_volume.get(), *m_sceneAdapter, plane));
    }
    return record;
}

QVector<DisplayMeasurement> VtkMprView::displayMeasurementsForPane(
    VtkSliceMprPaneView& pane,
    MprSlicePlane plane) const
{
    QVector<DisplayMeasurement> displayMeasurements;
    const auto appendMeasurement = [&](const MeasurementAnnotation& measurement, bool preview) {
        DisplayMeasurement display;
        display.type = measurement.type;
        display.color = measurement.color;
        display.label = measurementLabel(measurement, plane);
        display.preview = preview;
        const auto appendWorldPoint = [&](const MeasurementPoint& point) {
            const QPointF normalizedPosition = m_sceneAdapter->normalizedDisplayPositionForCursorWorld(
                plane,
                {point.x, point.y, point.z},
                pane.renderWidget()->size());
            display.points.append({
                normalizedPosition.x() * pane.renderWidget()->width(),
                normalizedPosition.y() * pane.renderWidget()->height()});
        };

        if (measurement.type == MeasurementType::RectangleRoi && measurement.points.size() >= 2)
        {
            const MeasurementPoint& first = measurement.points[0];
            const MeasurementPoint& second = measurement.points[1];
            MeasurementPoint topRight = first;
            MeasurementPoint bottomLeft = second;
            switch (plane)
            {
            case MprSlicePlane::Axial:
                topRight.x = second.x;
                topRight.y = first.y;
                topRight.z = first.z;
                bottomLeft.x = first.x;
                bottomLeft.y = second.y;
                bottomLeft.z = first.z;
                break;
            case MprSlicePlane::Coronal:
                topRight.x = second.x;
                topRight.y = first.y;
                topRight.z = first.z;
                bottomLeft.x = first.x;
                bottomLeft.y = first.y;
                bottomLeft.z = second.z;
                break;
            case MprSlicePlane::Sagittal:
                topRight.x = first.x;
                topRight.y = second.y;
                topRight.z = first.z;
                bottomLeft.x = first.x;
                bottomLeft.y = first.y;
                bottomLeft.z = second.z;
                break;
            }

            appendWorldPoint(first);
            appendWorldPoint(topRight);
            appendWorldPoint(second);
            appendWorldPoint(bottomLeft);
            display.closedShape = true;
            display.filled = true;
            if (display.points.size() >= 2)
            {
                display.labelAnchor = display.points[1] + QPointF(8.0, -8.0);
            }
        }
        else
        {
            display.points.reserve(measurement.points.size());
            for (const auto& point : measurement.points)
            {
                appendWorldPoint(point);
            }
            if (!display.points.isEmpty())
            {
                display.labelAnchor = display.points.last() + QPointF(8.0, -8.0);
            }
        }
        displayMeasurements.append(display);
    };

    for (const auto& measurement : m_measurementService.measurements())
    {
        if (measurementBelongsToCurrentSlice(measurement, plane))
        {
            appendMeasurement(measurement, false);
        }
    }
    if (const auto activeMeasurement = m_measurementService.activeMeasurement())
    {
        if (measurementBelongsToCurrentSlice(*activeMeasurement, plane))
        {
            appendMeasurement(*activeMeasurement, true);
        }
    }
    return displayMeasurements;
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
            normalizedCrosshairPositionForPane(*m_axialPane, MprSlicePlane::Axial, position));
        m_coronalPane->setCrosshairPosition(
            normalizedCrosshairPositionForPane(*m_coronalPane, MprSlicePlane::Coronal, position));
        m_sagittalPane->setCrosshairPosition(
            normalizedCrosshairPositionForPane(*m_sagittalPane, MprSlicePlane::Sagittal, position));
        updatePaneStatusText();
        refreshMeasurementOverlays();
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
