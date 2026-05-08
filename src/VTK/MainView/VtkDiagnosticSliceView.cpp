#include "VTK/MainView/VtkDiagnosticSliceView.h"

#include "Model/DicomImage.h"
#include "Model/MedicalImage.h"
#include "ViewerOverlays/MeasurementOverlayWidget.h"
#include "ViewerTools/Measurements/MeasurementAnalyticsService.h"
#include "ViewerTools/Measurements/MeasurementTool.h"
#include "ViewerTools/PanTool.h"
#include "ViewerTools/ViewerInputEvent.h"
#include "VTK/MainView/DicomMeasurementScalarSource.h"
#include "VTK/MainView/VtkSliceSceneAdapter.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMouseEvent>
#include <QSlider>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QVTKOpenGLNativeWidget.h>
#include <vtkNew.h>
#include <vtkPNGWriter.h>
#include <vtkRenderWindow.h>
#include <vtkUnsignedCharArray.h>
#include <vtkWindowToImageFilter.h>
#include <algorithm>

namespace
{
bool measurementPointsEqual(const QVector<MeasurementPoint>& left, const QVector<MeasurementPoint>& right)
{
    if (left.size() != right.size())
    {
        return false;
    }

    for (int index = 0; index < left.size(); ++index)
    {
        const MeasurementPoint& lhs = left[index];
        const MeasurementPoint& rhs = right[index];
        if (lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z)
        {
            return false;
        }
    }

    return true;
}

bool measurementAnnotationsEqual(const QVector<MeasurementAnnotation>& left, const QVector<MeasurementAnnotation>& right)
{
    if (left.size() != right.size())
    {
        return false;
    }

    for (int index = 0; index < left.size(); ++index)
    {
        const MeasurementAnnotation& lhs = left[index];
        const MeasurementAnnotation& rhs = right[index];
        if (lhs.id != rhs.id
            || lhs.type != rhs.type
            || lhs.color != rhs.color
            || lhs.lengthMm != rhs.lengthMm
            || !measurementPointsEqual(lhs.points, rhs.points))
        {
            return false;
        }
    }

    return true;
}
}

VtkDiagnosticSliceView::VtkDiagnosticSliceView(QWidget* parent)
    : QWidget(parent),
      m_sceneAdapter(std::make_unique<VtkSliceSceneAdapter>())
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    m_renderWidget = new QVTKOpenGLNativeWidget(this);
    m_renderWidget->setMinimumSize(320, 320);
    m_renderWidget->installEventFilter(this);
    layout->addWidget(m_renderWidget, 1);

    auto configureStatusLabel = [](QLabel* label) {
        label->setAttribute(Qt::WA_TransparentForMouseEvents);
        label->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
        label->setStyleSheet(
            "color: rgba(245, 247, 250, 235);"
            "background-color: rgba(0, 0, 0, 220);"
            "padding: 2px 6px;"
            "border-radius: 4px;");
        label->hide();
    };

    m_sliceInfoLabel = new QLabel(m_renderWidget);
    configureStatusLabel(m_sliceInfoLabel);

    m_windowLevelLabel = new QLabel(m_renderWidget);
    configureStatusLabel(m_windowLevelLabel);

    m_zoomLabel = new QLabel(m_renderWidget);
    configureStatusLabel(m_zoomLabel);

    m_measurementOverlay = new MeasurementOverlayWidget(m_renderWidget);
    m_measurementOverlay->setGeometry(m_renderWidget->rect());
    m_measurementOverlay->raise();
    m_measurementOverlay->show();

    buildControls();
    layout->addWidget(m_cineBar);

    m_sceneAdapter->attach(*m_renderWidget);
    m_distanceMeasurementTool = std::make_unique<MeasurementTool>(
        MeasurementToolMode::Distance,
        m_measurementService,
        *this);
    m_polylineMeasurementTool = std::make_unique<MeasurementTool>(
        MeasurementToolMode::Polyline,
        m_measurementService,
        *this);
    m_angleMeasurementTool = std::make_unique<MeasurementTool>(
        MeasurementToolMode::Angle,
        m_measurementService,
        *this);
    m_rectangleRoiMeasurementTool = std::make_unique<MeasurementTool>(
        MeasurementToolMode::RectangleRoi,
        m_measurementService,
        *this);
    m_panTool = std::make_unique<PanTool>(*this);
}

VtkDiagnosticSliceView::~VtkDiagnosticSliceView() = default;

std::int64_t VtkDiagnosticSliceView::currentImageByteCount() const
{
    return m_sceneAdapter ? m_sceneAdapter->currentImageByteCount() : 0;
}

void VtkDiagnosticSliceView::setImage(std::shared_ptr<MedicalImage> image, bool resetCamera)
{
    if (!image || !image->isValid())
    {
        clearImage();
        return;
    }

    if (const auto dicomImage = std::dynamic_pointer_cast<DicomImage>(image);
        dicomImage && dicomImage->hasRawPixels())
    {
        setDicomImage(*dicomImage, dicomImage->defaultWindowLevel(), dicomImage->defaultWindowWidth(), resetCamera);
        return;
    }

    m_currentDicomImage = nullptr;
    m_sceneAdapter->setQImage(image->pixmap().toImage(), resetCamera);
    updateStatusText();
    refreshMeasurementOverlay();
}

void VtkDiagnosticSliceView::setDicomImage(const DicomImage& image, int windowLevel, int windowWidth, bool resetCamera)
{
    if (!image.isValid() || !image.hasRawPixels())
    {
        clearImage();
        return;
    }

    m_sceneAdapter->setDicomImage(image, windowLevel, windowWidth, resetCamera);
    m_currentDicomImage = &image;
    setWindowLevelWidth(windowLevel, windowWidth);
    updateStatusText();
    refreshMeasurementOverlay();
}

void VtkDiagnosticSliceView::clearImage()
{
    m_sceneAdapter->clear();
    m_currentDicomImage = nullptr;
    m_currentSeriesInstanceUid.clear();
    m_currentSopInstanceUid.clear();
    clearMeasurements();
    m_sliceInfoLabel->hide();
    m_windowLevelLabel->hide();
    m_zoomLabel->hide();
    updateStatusText();
}

void VtkDiagnosticSliceView::setSliceNavigationState(int currentIndex, int totalCount)
{
    m_totalSliceCount = std::max(0, totalCount);
    m_currentSliceIndex = m_totalSliceCount > 0 ? std::clamp(currentIndex, 0, m_totalSliceCount - 1) : 0;

    m_sliceSlider->blockSignals(true);
    m_sliceSlider->setEnabled(m_totalSliceCount > 1);
    m_sliceSlider->setMinimum(0);
    m_sliceSlider->setMaximum(std::max(0, m_totalSliceCount - 1));
    m_sliceSlider->setValue(m_currentSliceIndex);
    m_sliceSlider->blockSignals(false);

    updateSliceNavigationLabel();
    updateStatusText();
    m_cineBar->setVisible(m_totalSliceCount > 0);
}

void VtkDiagnosticSliceView::setCineAvailable(bool available)
{
    m_cinePlayButton->setEnabled(available);
    if (!available)
    {
        m_cinePlayButton->blockSignals(true);
        m_cinePlayButton->setChecked(false);
        m_cinePlayButton->setText("Play");
        m_cinePlayButton->blockSignals(false);
    }
}

void VtkDiagnosticSliceView::setCinePlaying(bool playing)
{
    m_cinePlayButton->blockSignals(true);
    m_cinePlayButton->setChecked(playing);
    m_cinePlayButton->setText(playing ? "Pause" : "Play");
    m_cinePlayButton->blockSignals(false);
}

void VtkDiagnosticSliceView::setPatientInfoText(
    const QString& patientName,
    const QString& age,
    const QString& dateOfBirth,
    const QString& doctor,
    const QString& modality,
    const QString& scanDate)
{
    Q_UNUSED(patientName)
    Q_UNUSED(age)
    Q_UNUSED(dateOfBirth)
    Q_UNUSED(doctor)
    Q_UNUSED(modality)
    Q_UNUSED(scanDate)
}

void VtkDiagnosticSliceView::setWindowLevelWidth(int windowLevel, int windowWidth)
{
    m_currentWindowLevel = windowLevel;
    m_currentWindowWidth = windowWidth;
    updateStatusText();
}

void VtkDiagnosticSliceView::setWindowLevelInteractionEnabled(bool enabled)
{
    m_windowLevelInteractionEnabled = enabled;
    updateDragState(enabled, m_windowLevelDragActive);
}

void VtkDiagnosticSliceView::setZoomInteractionEnabled(bool enabled)
{
    m_zoomInteractionEnabled = enabled;
    updateDragState(enabled, m_zoomDragActive);
}

void VtkDiagnosticSliceView::setPanInteractionEnabled(bool enabled)
{
    m_panInteractionEnabled = enabled;
    updateDragState(enabled, m_panDragActive);
    updateCursorState();
}

void VtkDiagnosticSliceView::setDistanceMeasurementEnabled(bool enabled)
{
    if (enabled)
    {
        m_activeMeasurementTool = ActiveMeasurementTool::Distance;
        return;
    }

    if (m_activeMeasurementTool == ActiveMeasurementTool::Distance)
    {
        m_activeMeasurementTool = ActiveMeasurementTool::None;
        m_measurementService.cancelActiveMeasurement();
        refreshMeasurementOverlay();
    }
}

void VtkDiagnosticSliceView::setPolylineMeasurementEnabled(bool enabled)
{
    if (enabled)
    {
        m_activeMeasurementTool = ActiveMeasurementTool::Polyline;
        return;
    }

    if (m_activeMeasurementTool == ActiveMeasurementTool::Polyline)
    {
        m_activeMeasurementTool = ActiveMeasurementTool::None;
        m_measurementService.cancelActiveMeasurement();
        refreshMeasurementOverlay();
    }
}

void VtkDiagnosticSliceView::setAngleMeasurementEnabled(bool enabled)
{
    if (enabled)
    {
        m_activeMeasurementTool = ActiveMeasurementTool::Angle;
        return;
    }

    if (m_activeMeasurementTool == ActiveMeasurementTool::Angle)
    {
        m_activeMeasurementTool = ActiveMeasurementTool::None;
        m_measurementService.cancelActiveMeasurement();
        refreshMeasurementOverlay();
    }
}

void VtkDiagnosticSliceView::setRectangleRoiMeasurementEnabled(bool enabled)
{
    if (enabled)
    {
        m_activeMeasurementTool = ActiveMeasurementTool::RectangleRoi;
        return;
    }

    if (m_activeMeasurementTool == ActiveMeasurementTool::RectangleRoi)
    {
        m_activeMeasurementTool = ActiveMeasurementTool::None;
        m_measurementService.cancelActiveMeasurement();
        refreshMeasurementOverlay();
    }
}

void VtkDiagnosticSliceView::clearMeasurements()
{
    m_measurementService.clear();
    m_lastPersistedMeasurements.clear();
    refreshMeasurementOverlay();
}

void VtkDiagnosticSliceView::setSliceAnnotationContext(
    const QString& seriesInstanceUid,
    const QString& sopInstanceUid,
    int frameIndex)
{
    m_currentSeriesInstanceUid = seriesInstanceUid;
    m_currentSopInstanceUid = sopInstanceUid;
    m_currentFrameIndex = std::max(0, frameIndex);
}

void VtkDiagnosticSliceView::loadSliceAnnotations(const QList<SliceMeasurementAnnotationRecord>& records)
{
    QVector<MeasurementAnnotation> measurements;
    measurements.reserve(records.size());
    for (const SliceMeasurementAnnotationRecord& record : records)
    {
        measurements.append(record.measurement);
    }

    m_suppressSliceAnnotationSignal = true;
    m_measurementService.setMeasurements(measurements);
    m_lastPersistedMeasurements = measurements;
    refreshMeasurementOverlay();
    m_suppressSliceAnnotationSignal = false;
}

void VtkDiagnosticSliceView::applyZoomDelta(int delta)
{
    m_sceneAdapter->applyZoomDelta(delta);
    updateStatusText();
    refreshMeasurementOverlay();
}

QByteArray VtkDiagnosticSliceView::captureSnapshotPng() const
{
    if (!m_renderWidget || !m_renderWidget->renderWindow())
    {
        return {};
    }

    m_renderWidget->renderWindow()->Render();

    vtkNew<vtkWindowToImageFilter> windowToImageFilter;
    windowToImageFilter->SetInput(m_renderWidget->renderWindow());
    windowToImageFilter->SetInputBufferTypeToRGBA();
    windowToImageFilter->ReadFrontBufferOff();
    windowToImageFilter->Update();

    vtkNew<vtkPNGWriter> pngWriter;
    pngWriter->SetInputConnection(windowToImageFilter->GetOutputPort());
    pngWriter->WriteToMemoryOn();
    pngWriter->Write();

    vtkUnsignedCharArray* result = pngWriter->GetResult();
    if (!result || result->GetNumberOfValues() <= 0)
    {
        return {};
    }

    return QByteArray(
        reinterpret_cast<const char*>(result->GetPointer(0)),
        static_cast<qsizetype>(result->GetNumberOfValues()));
}

bool VtkDiagnosticSliceView::eventFilter(QObject* watched, QEvent* event)
{
    if (watched != m_renderWidget)
    {
        return QWidget::eventFilter(watched, event);
    }

    if (event->type() == QEvent::Resize || event->type() == QEvent::Show)
    {
        layoutOverlayWidgets();
        m_sceneAdapter->fitToView();
        updateStatusText();
        refreshMeasurementOverlay();
    }

    if (event->type() == QEvent::Wheel)
    {
        const auto* wheelEvent = static_cast<QWheelEvent*>(event);
        if (wheelEvent->angleDelta().y() != 0)
        {
            emit wheelSliceNavigationRequested(wheelEvent->angleDelta().y() > 0 ? -1 : 1);
            return true;
        }
    }

    if (activeMeasurementTool() && handleMeasurementEvent(event))
    {
        return true;
    }

    if (m_windowLevelInteractionEnabled && event->type() == QEvent::MouseButtonPress)
    {
        const auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton)
        {
            m_windowLevelDragActive = true;
            m_lastWindowLevelDragPosition = mouseEvent->position().toPoint();
            return true;
        }
    }

    if (m_zoomInteractionEnabled && event->type() == QEvent::MouseButtonPress)
    {
        const auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton)
        {
            m_zoomDragActive = true;
            m_lastZoomDragPosition = mouseEvent->position().toPoint();
            return true;
        }
    }

    if (m_panInteractionEnabled && event->type() == QEvent::MouseButtonPress)
    {
        const auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton)
        {
            m_panDragActive = true;
            m_lastPanDragPosition = mouseEvent->position().toPoint();
            updateCursorState();
            m_panTool->beginInteraction(makePanViewerInputEvent(
                ViewerInputEvent::Phase::Begin,
                MprSlicePlane::Axial,
                mouseEvent->button(),
                mouseEvent->position(),
                {},
                m_renderWidget->size()));
            return true;
        }
    }

    if (m_windowLevelInteractionEnabled && event->type() == QEvent::MouseMove && m_windowLevelDragActive)
    {
        const auto* mouseEvent = static_cast<QMouseEvent*>(event);
        const QPoint currentPosition = mouseEvent->position().toPoint();
        const QPoint delta = currentPosition - m_lastWindowLevelDragPosition;
        m_lastWindowLevelDragPosition = currentPosition;
        if (!delta.isNull())
        {
            emit windowLevelDragDelta(-delta.y(), delta.x());
        }
        return true;
    }

    if (m_zoomInteractionEnabled && event->type() == QEvent::MouseMove && m_zoomDragActive)
    {
        const auto* mouseEvent = static_cast<QMouseEvent*>(event);
        const QPoint currentPosition = mouseEvent->position().toPoint();
        const QPoint delta = currentPosition - m_lastZoomDragPosition;
        m_lastZoomDragPosition = currentPosition;
        if (delta.y() != 0)
        {
            applyZoomDelta(-delta.y());
        }
        return true;
    }

    if (m_panInteractionEnabled && event->type() == QEvent::MouseMove && m_panDragActive)
    {
        const auto* mouseEvent = static_cast<QMouseEvent*>(event);
        const QPoint currentPosition = mouseEvent->position().toPoint();
        const QPoint delta = currentPosition - m_lastPanDragPosition;
        m_lastPanDragPosition = currentPosition;
        if (!delta.isNull())
        {
            m_panTool->updateInteraction(makePanViewerInputEvent(
                ViewerInputEvent::Phase::Update,
                MprSlicePlane::Axial,
                Qt::NoButton,
                mouseEvent->position(),
                delta,
                m_renderWidget->size()));
        }
        return true;
    }

    if (event->type() == QEvent::MouseButtonRelease)
    {
        const auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton && m_windowLevelDragActive)
        {
            m_windowLevelDragActive = false;
            return m_windowLevelInteractionEnabled;
        }
        if (mouseEvent->button() == Qt::LeftButton && m_zoomDragActive)
        {
            m_zoomDragActive = false;
            return m_zoomInteractionEnabled;
        }
        if (mouseEvent->button() == Qt::LeftButton && m_panDragActive)
        {
            m_panDragActive = false;
            updateCursorState();
            m_panTool->endInteraction(makePanViewerInputEvent(
                ViewerInputEvent::Phase::End,
                MprSlicePlane::Axial,
                mouseEvent->button(),
                mouseEvent->position(),
                {},
                m_renderWidget->size()));
            return m_panInteractionEnabled;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void VtkDiagnosticSliceView::updateDragState(bool enabled, bool& dragActive)
{
    if (!enabled)
    {
        dragActive = false;
    }
}

void VtkDiagnosticSliceView::updateCursorState()
{
    if (!m_renderWidget)
    {
        return;
    }

    if (m_panDragActive)
    {
        m_renderWidget->setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (m_panInteractionEnabled)
    {
        m_renderWidget->setCursor(Qt::OpenHandCursor);
        return;
    }

    m_renderWidget->unsetCursor();
}

bool VtkDiagnosticSliceView::handleMeasurementEvent(QEvent* event)
{
    IViewerTool* tool = activeMeasurementTool();
    if (!tool)
    {
        return false;
    }

    ViewerInputEvent inputEvent;
    inputEvent.plane = MprSlicePlane::Axial;
    inputEvent.widgetSize = m_renderWidget ? m_renderWidget->size() : QSize{};

    switch (event->type())
    {
    case QEvent::MouseMove:
    {
        const auto* mouseEvent = static_cast<QMouseEvent*>(event);
        inputEvent.eventType = ViewerInputEvent::EventType::MouseMove;
        inputEvent.phase = ViewerInputEvent::Phase::Update;
        inputEvent.button = Qt::NoButton;
        inputEvent.displayPosition = mouseEvent->position();
        tool->updateInteraction(inputEvent);
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
        tool->beginInteraction(inputEvent);
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
        tool->endInteraction(inputEvent);
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
        tool->beginInteraction(inputEvent);
        event->accept();
        return true;
    }
    default:
        break;
    }

    return false;
}

MeasurementPoint VtkDiagnosticSliceView::measurementPointForInput(const ViewerInputEvent& event) const
{
    return measurementPointForMousePosition(event.displayPosition);
}

void VtkDiagnosticSliceView::onMeasurementToolUpdated()
{
    refreshMeasurementOverlay();
    emitSliceAnnotationsChangedIfNeeded();
}

void VtkDiagnosticSliceView::handleCrosshairInput(const ViewerInputEvent& event)
{
    Q_UNUSED(event);
}

void VtkDiagnosticSliceView::handleWindowLevelInput(const ViewerInputEvent& event)
{
    Q_UNUSED(event);
}

void VtkDiagnosticSliceView::handleZoomInput(const ViewerInputEvent& event)
{
    Q_UNUSED(event);
}

void VtkDiagnosticSliceView::handlePanInput(const ViewerInputEvent& event)
{
    if (event.phase != ViewerInputEvent::Phase::Update)
    {
        return;
    }

    m_sceneAdapter->pan(event.displayDelta, event.widgetSize);
    refreshMeasurementOverlay();
    updateStatusText();
}

void VtkDiagnosticSliceView::refreshMeasurementOverlay()
{
    if (!m_measurementOverlay || !m_renderWidget)
    {
        return;
    }

    m_measurementOverlay->setGeometry(m_renderWidget->rect());
    m_measurementOverlay->setMeasurements(displayMeasurements());
    m_measurementOverlay->raise();
}

void VtkDiagnosticSliceView::emitSliceAnnotationsChangedIfNeeded()
{
    if (m_suppressSliceAnnotationSignal
        || m_currentSeriesInstanceUid.isEmpty()
        || m_currentSopInstanceUid.isEmpty())
    {
        return;
    }

    const QVector<MeasurementAnnotation>& currentMeasurements = m_measurementService.measurements();
    if (measurementAnnotationsEqual(currentMeasurements, m_lastPersistedMeasurements))
    {
        return;
    }

    m_lastPersistedMeasurements = currentMeasurements;
    emit sliceAnnotationsChanged(currentSliceAnnotationRecords());
}

IViewerTool* VtkDiagnosticSliceView::activeMeasurementTool() const
{
    switch (m_activeMeasurementTool)
    {
    case ActiveMeasurementTool::Distance:
        return m_distanceMeasurementTool.get();
    case ActiveMeasurementTool::Polyline:
        return m_polylineMeasurementTool.get();
    case ActiveMeasurementTool::Angle:
        return m_angleMeasurementTool.get();
    case ActiveMeasurementTool::RectangleRoi:
        return m_rectangleRoiMeasurementTool.get();
    case ActiveMeasurementTool::None:
        break;
    }

    return nullptr;
}

MeasurementPoint VtkDiagnosticSliceView::measurementPointForMousePosition(const QPointF& position) const
{
    return m_sceneAdapter->measurementPointFromDisplayPosition(position, m_renderWidget->size());
}

QString VtkDiagnosticSliceView::measurementLabel(const MeasurementAnnotation& measurement) const
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
            DicomMeasurementScalarSource(m_currentDicomImage, *m_sceneAdapter));
    }

    return {};
}

QVector<DisplayMeasurement> VtkDiagnosticSliceView::displayMeasurements() const
{
    QVector<DisplayMeasurement> displayMeasurements;
    const auto appendMeasurement = [&](const MeasurementAnnotation& measurement, bool preview) {
        DisplayMeasurement display;
        display.type = measurement.type;
        display.color = measurement.color;
        display.label = measurementLabel(measurement);
        display.preview = preview;
        if (measurement.type == MeasurementType::RectangleRoi && measurement.points.size() >= 2)
        {
            const QPointF first = m_sceneAdapter->imageIndexForMeasurementPoint(measurement.points[0]);
            const QPointF second = m_sceneAdapter->imageIndexForMeasurementPoint(measurement.points[1]);
            const QPointF topLeft(std::min(first.x(), second.x()), std::min(first.y(), second.y()));
            const QPointF bottomRight(std::max(first.x(), second.x()), std::max(first.y(), second.y()));
            const QPointF topRight(bottomRight.x(), topLeft.y());
            const QPointF bottomLeft(topLeft.x(), bottomRight.y());
            display.points = {
                m_sceneAdapter->displayPositionForImageIndex(topLeft, m_renderWidget->size()),
                m_sceneAdapter->displayPositionForImageIndex(topRight, m_renderWidget->size()),
                m_sceneAdapter->displayPositionForImageIndex(bottomRight, m_renderWidget->size()),
                m_sceneAdapter->displayPositionForImageIndex(bottomLeft, m_renderWidget->size())};
            display.closedShape = true;
            display.filled = true;
            display.labelAnchor = display.points[1] + QPointF(8.0, -8.0);
        }
        else
        {
            display.points.reserve(measurement.points.size());
            for (const auto& point : measurement.points)
            {
                display.points.append(m_sceneAdapter->displayPositionForMeasurementPoint(point, m_renderWidget->size()));
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
        appendMeasurement(measurement, false);
    }
    if (const auto activeMeasurement = m_measurementService.activeMeasurement())
    {
        appendMeasurement(*activeMeasurement, true);
    }
    return displayMeasurements;
}

QList<SliceMeasurementAnnotationRecord> VtkDiagnosticSliceView::currentSliceAnnotationRecords() const
{
    QList<SliceMeasurementAnnotationRecord> records;
    if (m_currentSeriesInstanceUid.isEmpty() || m_currentSopInstanceUid.isEmpty())
    {
        return records;
    }

    records.reserve(m_measurementService.measurements().size());
    for (const MeasurementAnnotation& measurement : m_measurementService.measurements())
    {
        SliceMeasurementAnnotationRecord record;
        record.seriesInstanceUid = m_currentSeriesInstanceUid;
        record.sopInstanceUid = m_currentSopInstanceUid;
        record.frameIndex = m_currentFrameIndex;
        record.measurement = measurement;
        if (measurement.type == MeasurementType::Angle)
        {
            record.angleDegrees = MeasurementService::angleDegrees(measurement.points);
        }
        else if (measurement.type == MeasurementType::RectangleRoi && m_currentDicomImage)
        {
            record.roiStatistics = MeasurementAnalyticsService::rectangleRoiStatistics(
                measurement,
                DicomMeasurementScalarSource(m_currentDicomImage, *m_sceneAdapter));
        }
        records.append(record);
    }

    return records;
}

void VtkDiagnosticSliceView::buildControls()
{
    m_cineBar = new QWidget(this);

    auto* layout = new QHBoxLayout(m_cineBar);
    layout->setContentsMargins(6, 0, 6, 6);
    layout->setSpacing(8);

    m_cinePlayButton = new QToolButton(m_cineBar);
    m_cinePlayButton->setCheckable(true);
    m_cinePlayButton->setText("Play");
    connect(m_cinePlayButton, &QToolButton::toggled, this, [this](bool checked) {
        m_cinePlayButton->setText(checked ? "Pause" : "Play");
        emit cinePlaybackToggled(checked);
    });

    m_sliceSlider = new QSlider(Qt::Horizontal, m_cineBar);
    m_sliceSlider->setRange(0, 0);
    m_sliceSlider->setTickPosition(QSlider::TicksBelow);
    m_sliceSlider->setTickInterval(1);
    connect(m_sliceSlider, &QSlider::valueChanged, this, &VtkDiagnosticSliceView::sliceIndexSelected);

    m_sliceLabel = new QLabel("0 / 0", m_cineBar);

    layout->addWidget(m_cinePlayButton);
    layout->addWidget(m_sliceSlider, 1);
    layout->addWidget(m_sliceLabel);

    m_cineBar->setVisible(false);
}

void VtkDiagnosticSliceView::updateSliceNavigationLabel()
{
    const int currentDisplayIndex = m_totalSliceCount > 0 ? m_currentSliceIndex + 1 : 0;
    m_sliceLabel->setText(QString("%1 / %2").arg(currentDisplayIndex).arg(m_totalSliceCount));
}

void VtkDiagnosticSliceView::updateStatusText()
{
    const int currentDisplayIndex = m_totalSliceCount > 0 ? m_currentSliceIndex + 1 : 0;
    m_sliceInfoLabel->setText(QString("Slice: %1 / %2").arg(currentDisplayIndex).arg(m_totalSliceCount));
    m_windowLevelLabel->setText(QString("WL/WW: %1 / %2").arg(m_currentWindowLevel).arg(m_currentWindowWidth));
    m_zoomLabel->setText(QString("Zoom: %1%").arg(m_sceneAdapter->zoomPercent()));

    const bool showStatus = m_totalSliceCount > 0;
    m_sliceInfoLabel->setVisible(showStatus);
    m_windowLevelLabel->setVisible(showStatus);
    m_zoomLabel->setVisible(showStatus);
    layoutOverlayWidgets();
}

void VtkDiagnosticSliceView::layoutOverlayWidgets()
{
    constexpr int margin = 8;
    constexpr int spacing = 4;

    int y = m_renderWidget->height() - margin;
    const auto positionStatusLabel = [&](QLabel* label) {
        if (!label || !label->isVisible())
        {
            return;
        }

        label->adjustSize();
        y -= label->height();
        label->move(margin, std::max(margin, y));
        label->raise();
        y -= spacing;
    };

    positionStatusLabel(m_zoomLabel);
    positionStatusLabel(m_windowLevelLabel);
    positionStatusLabel(m_sliceInfoLabel);
    refreshMeasurementOverlay();
}
