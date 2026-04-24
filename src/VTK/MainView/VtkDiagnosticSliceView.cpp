#include "VTK/MainView/VtkDiagnosticSliceView.h"

#include "Model/DicomImage.h"
#include "Model/MedicalImage.h"
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

    m_patientInfoLabel = new QLabel(m_renderWidget);
    m_patientInfoLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_patientInfoLabel->setAlignment(Qt::AlignRight | Qt::AlignTop);
    m_patientInfoLabel->setStyleSheet(
        "color: rgba(245, 247, 250, 235);"
        "background-color: rgba(0, 0, 0, 220);"
        "padding: 4px 8px;"
        "border-radius: 4px;");
    m_patientInfoLabel->hide();

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

    buildControls();
    layout->addWidget(m_cineBar);

    m_sceneAdapter->attach(*m_renderWidget);
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

    m_sceneAdapter->setQImage(image->pixmap().toImage(), resetCamera);
    updateStatusText();
}

void VtkDiagnosticSliceView::setDicomImage(const DicomImage& image, int windowLevel, int windowWidth, bool resetCamera)
{
    if (!image.isValid() || !image.hasRawPixels())
    {
        clearImage();
        return;
    }

    m_sceneAdapter->setDicomImage(image, windowLevel, windowWidth, resetCamera);
    setWindowLevelWidth(windowLevel, windowWidth);
    updateStatusText();
}

void VtkDiagnosticSliceView::clearImage()
{
    m_sceneAdapter->clear();
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
    QStringList lines;
    if (!patientName.trimmed().isEmpty())
    {
        lines << QString("Name: %1").arg(patientName.trimmed());
    }
    if (!age.trimmed().isEmpty())
    {
        lines << QString("Age: %1").arg(age.trimmed());
    }
    if (!dateOfBirth.trimmed().isEmpty())
    {
        lines << QString("DOB: %1").arg(dateOfBirth.trimmed());
    }
    if (!doctor.trimmed().isEmpty())
    {
        lines << QString("Doctor: %1").arg(doctor.trimmed());
    }
    if (!modality.trimmed().isEmpty())
    {
        lines << QString("Modality: %1").arg(modality.trimmed());
    }
    if (!scanDate.trimmed().isEmpty())
    {
        lines << QString("Scan Date: %1").arg(scanDate.trimmed());
    }

    if (lines.isEmpty())
    {
        m_patientInfoLabel->clear();
        m_patientInfoLabel->hide();
        return;
    }

    m_patientInfoLabel->setText(lines.join('\n'));
    m_patientInfoLabel->setVisible(true);
    layoutOverlayWidgets();
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

void VtkDiagnosticSliceView::applyZoomDelta(int delta)
{
    m_sceneAdapter->applyZoomDelta(delta);
    updateStatusText();
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

    if (m_patientInfoLabel && m_patientInfoLabel->isVisible())
    {
        m_patientInfoLabel->adjustSize();
        m_patientInfoLabel->move(
            std::max(margin, m_renderWidget->width() - m_patientInfoLabel->width() - margin),
            margin);
        m_patientInfoLabel->raise();
    }

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
}
