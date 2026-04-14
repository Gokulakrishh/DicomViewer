#include "MprViewerWindow.h"

#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QSignalBlocker>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <array>
#include <cmath>

#include "Model/IVolumeData.h"

namespace
{
struct WindowPreset
{
    const char* name;
    int level;
    int width;
};

constexpr std::array<WindowPreset, 4> kWindowPresets{{
    {"Brain", 40, 80},
    {"Soft Tissue", 40, 400},
    {"Bone", 300, 1500},
    {"Lung", -600, 1500},
}};

MprViewerWindow::PaneWidgets createPane(
    MprRenderService::Plane plane,
    const QString& title,
    int maxSliceIndex,
    QWidget* parent,
    QBoxLayout* parentLayout)
{
    MprViewerWindow::PaneWidgets pane;
    pane.plane = plane;
    auto* container = new QWidget(parent);
    pane.container = container;
    auto* layout = new QVBoxLayout(container);

    pane.titleLabel = new QLabel(title, container);
    pane.imageLabel = new QLabel(container);
    pane.imageLabel->setAlignment(Qt::AlignCenter);
    pane.imageLabel->setMinimumSize(220, 220);
    pane.imageLabel->setFrameShape(QFrame::StyledPanel);
    pane.imageLabel->setMouseTracking(true);
    pane.slider = new QSlider(Qt::Horizontal, container);
    pane.slider->setMinimum(0);
    pane.slider->setMaximum(maxSliceIndex);
    pane.slider->setValue(maxSliceIndex / 2);

    layout->addWidget(pane.titleLabel);
    layout->addWidget(pane.imageLabel, 1);
    layout->addWidget(pane.slider);
    parentLayout->addWidget(container, 1);
    return pane;
}
}

MprViewerWindow::MprViewerWindow(
    std::shared_ptr<IVolumeData> volume,
    int initialWindowLevel,
    int initialWindowWidth,
    QWidget* parent)
    : QMainWindow(parent),
      m_sourceVolume(std::move(volume)),
      m_windowLevel(initialWindowLevel),
      m_windowWidth(std::max(1, initialWindowWidth))
{
    const auto& sourceGeometry = m_sourceVolume->geometry();
    qDebug().nospace()
        << "MprViewerWindow source volume:"
        << " dims=(" << sourceGeometry.dimensions.x << ", " << sourceGeometry.dimensions.y << ", " << sourceGeometry.dimensions.z << ")"
        << " spacing=(" << sourceGeometry.spacing.x << ", " << sourceGeometry.spacing.y << ", " << sourceGeometry.spacing.z << ")";

    m_displayVolume = m_resampleService.resampleIsotropic(*m_sourceVolume);
    if (!m_displayVolume)
    {
        m_displayVolume = m_sourceVolume;
    }

    const auto& geometry = m_displayVolume->geometry();
    qDebug().nospace()
        << "MprViewerWindow display volume:"
        << " dims=(" << geometry.dimensions.x << ", " << geometry.dimensions.y << ", " << geometry.dimensions.z << ")"
        << " spacing=(" << geometry.spacing.x << ", " << geometry.spacing.y << ", " << geometry.spacing.z << ")"
        << " origin=(" << geometry.origin.x << ", " << geometry.origin.y << ", " << geometry.origin.z << ")";
    qDebug() << "Inferior-most world z:" << geometry.origin.z;
    qDebug() << "Superior-most world z:" << (geometry.origin.z + ((geometry.dimensions.z - 1) * geometry.spacing.z));
    qDebug() << "Displayed coronal height:" << geometry.dimensions.z;
    qDebug() << "Displayed sagittal height:" << geometry.dimensions.z;
    m_crosshair = {
        std::max(0, geometry.dimensions.x / 2),
        std::max(0, geometry.dimensions.y / 2),
        std::max(0, geometry.dimensions.z / 2)};
    setupUi();
    renderAllPanes();
}

void MprViewerWindow::setupUi()
{
    auto* central = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(central);
    auto* controlsLayout = new QHBoxLayout();
    m_paneLayout = new QHBoxLayout();
    const auto& geometry = m_displayVolume->geometry();

    m_axialPane = createPane(MprRenderService::Plane::Axial, "Axial", geometry.dimensions.z - 1, central, m_paneLayout);
    m_coronalPane = createPane(MprRenderService::Plane::Coronal, "Coronal", geometry.dimensions.y - 1, central, m_paneLayout);
    m_sagittalPane = createPane(MprRenderService::Plane::Sagittal, "Sagittal", geometry.dimensions.x - 1, central, m_paneLayout);

    controlsLayout->addWidget(new QLabel("Preset", central));
    m_presetComboBox = new QComboBox(central);
    for (const auto& preset : kWindowPresets)
    {
        m_presetComboBox->addItem(QString::fromLatin1(preset.name));
    }
    controlsLayout->addWidget(m_presetComboBox);

    controlsLayout->addWidget(new QLabel("WL", central));
    m_windowLevelSlider = new QSlider(Qt::Horizontal, central);
    m_windowLevelSlider->setRange(-2000, 2000);
    m_windowLevelSlider->setValue(m_windowLevel);
    controlsLayout->addWidget(m_windowLevelSlider, 1);
    m_windowLevelLabel = new QLabel(central);
    controlsLayout->addWidget(m_windowLevelLabel);

    controlsLayout->addWidget(new QLabel("WW", central));
    m_windowWidthSlider = new QSlider(Qt::Horizontal, central);
    m_windowWidthSlider->setRange(1, 4000);
    m_windowWidthSlider->setValue(m_windowWidth);
    controlsLayout->addWidget(m_windowWidthSlider, 1);
    m_windowWidthLabel = new QLabel(central);
    controlsLayout->addWidget(m_windowWidthLabel);

    m_coordinateLabel = new QLabel(central);
    m_coordinateLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_coordinateLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    rootLayout->addLayout(controlsLayout);
    rootLayout->addLayout(m_paneLayout, 1);
    rootLayout->addWidget(m_coordinateLabel);

    setCentralWidget(central);
    resize(1200, 480);

    updatePaneStretchFactors();

    m_axialPane.imageLabel->installEventFilter(this);
    m_coronalPane.imageLabel->installEventFilter(this);
    m_sagittalPane.imageLabel->installEventFilter(this);

    connect(m_axialPane.slider, &QSlider::valueChanged, this, [this](int value) {
        m_crosshair.z = value;
        renderAllPanes();
    });
    connect(m_coronalPane.slider, &QSlider::valueChanged, this, [this](int value) {
        m_crosshair.y = value;
        renderAllPanes();
    });
    connect(m_sagittalPane.slider, &QSlider::valueChanged, this, [this](int value) {
        m_crosshair.x = value;
        renderAllPanes();
    });

    connect(m_presetComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        applyWindowPreset(index);
    });
    connect(m_windowLevelSlider, &QSlider::valueChanged, this, [this](int value) {
        m_windowLevel = value;
        updateWindowControlLabels();
        renderAllPanes();
    });
    connect(m_windowWidthSlider, &QSlider::valueChanged, this, [this](int value) {
        m_windowWidth = value;
        updateWindowControlLabels();
        renderAllPanes();
    });

    syncSlidersToCrosshair();
    updateWindowControlLabels();
    m_presetComboBox->setCurrentIndex(-1);
}

void MprViewerWindow::renderAllPanes()
{
    syncSlidersToCrosshair();
    updateCoordinateReadout();
    renderPane(MprRenderService::Plane::Axial, m_axialPane);
    renderPane(MprRenderService::Plane::Coronal, m_coronalPane);
    renderPane(MprRenderService::Plane::Sagittal, m_sagittalPane);
}

void MprViewerWindow::renderPane(MprRenderService::Plane plane, PaneWidgets& pane)
{
    if (!m_displayVolume)
    {
        pane.imageLabel->clear();
        return;
    }

    const QImage image = m_renderService.renderSlice(
        *m_displayVolume,
        plane,
        crosshairSliceIndex(plane),
        m_windowLevel,
        m_windowWidth);
    if (image.isNull())
    {
        pane.imageLabel->setText("No slice");
        return;
    }

    pane.sourceImageSize = image.size();

    QImage overlayImage = image.convertToFormat(QImage::Format_ARGB32);
    QPainter painter(&overlayImage);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(QPen(QColor(255, 64, 64), 1));
    const QPoint crosshair = crosshairPixel(plane, overlayImage.size());
    painter.drawLine(crosshair.x(), 0, crosshair.x(), overlayImage.height() - 1);
    painter.drawLine(0, crosshair.y(), overlayImage.width() - 1, crosshair.y());

    painter.setPen(QPen(QColor(255, 255, 0), 1));
    painter.drawEllipse(crosshair, 3, 3);
    painter.end();

    pane.imageLabel->setPixmap(
        QPixmap::fromImage(overlayImage).scaled(
            pane.imageLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
}

void MprViewerWindow::updatePaneStretchFactors()
{
    if (!m_paneLayout)
    {
        return;
    }

    if (m_axialPane.container)
    {
        m_paneLayout->setStretch(m_paneLayout->indexOf(m_axialPane.container), paneStretchFactor(m_axialPane.plane));
    }
    if (m_coronalPane.container)
    {
        m_paneLayout->setStretch(m_paneLayout->indexOf(m_coronalPane.container), paneStretchFactor(m_coronalPane.plane));
    }
    if (m_sagittalPane.container)
    {
        m_paneLayout->setStretch(m_paneLayout->indexOf(m_sagittalPane.container), paneStretchFactor(m_sagittalPane.plane));
    }
}

int MprViewerWindow::paneStretchFactor(MprRenderService::Plane plane) const
{
    const QSize voxelSize = planeVoxelSize(plane);
    if (voxelSize.height() <= 0)
    {
        return 1;
    }

    const double aspectRatio = static_cast<double>(voxelSize.width()) /
                               static_cast<double>(voxelSize.height());
    return std::max(1, static_cast<int>(std::lround(aspectRatio * 100.0)));
}

void MprViewerWindow::syncSlidersToCrosshair()
{
    {
        const QSignalBlocker blocker(m_axialPane.slider);
        m_axialPane.slider->setValue(m_crosshair.z);
    }
    {
        const QSignalBlocker blocker(m_coronalPane.slider);
        m_coronalPane.slider->setValue(m_crosshair.y);
    }
    {
        const QSignalBlocker blocker(m_sagittalPane.slider);
        m_sagittalPane.slider->setValue(m_crosshair.x);
    }
}

int MprViewerWindow::crosshairSliceIndex(MprRenderService::Plane plane) const
{
    switch (plane)
    {
    case MprRenderService::Plane::Axial:
        return m_crosshair.z;
    case MprRenderService::Plane::Coronal:
        return m_crosshair.y;
    case MprRenderService::Plane::Sagittal:
        return m_crosshair.x;
    }

    return 0;
}

QSize MprViewerWindow::planeVoxelSize(MprRenderService::Plane plane) const
{
    if (!m_displayVolume)
    {
        return {};
    }

    const auto& geometry = m_displayVolume->geometry();
    switch (plane)
    {
    case MprRenderService::Plane::Axial:
        return {geometry.dimensions.x, geometry.dimensions.y};
    case MprRenderService::Plane::Coronal:
        return {geometry.dimensions.x, geometry.dimensions.z};
    case MprRenderService::Plane::Sagittal:
        return {geometry.dimensions.y, geometry.dimensions.z};
    }

    return {};
}

void MprViewerWindow::updateCoordinateReadout()
{
    if (!m_coordinateLabel || !m_displayVolume)
    {
        return;
    }

    const VolumeVector3D world = currentWorldCoordinate();
    const double scalarValue = currentScalarValue();
    m_coordinateLabel->setText(QString(
                                   "Voxel: x=%1  y=%2  z=%3    World: x=%4 mm  y=%5 mm  z=%6 mm    Value: %7")
                                   .arg(m_crosshair.x)
                                   .arg(m_crosshair.y)
                                   .arg(m_crosshair.z)
                                   .arg(world.x, 0, 'f', 2)
                                   .arg(world.y, 0, 'f', 2)
                                   .arg(world.z, 0, 'f', 2)
                                   .arg(scalarValue, 0, 'f', 1));
}

double MprViewerWindow::currentScalarValue() const
{
    if (!m_displayVolume || !m_displayVolume->isValidIndex(m_crosshair.x, m_crosshair.y, m_crosshair.z))
    {
        return 0.0;
    }

    return m_displayVolume->scalarAt(m_crosshair.x, m_crosshair.y, m_crosshair.z);
}

void MprViewerWindow::updateWindowControlLabels()
{
    if (m_windowLevelLabel)
    {
        m_windowLevelLabel->setText(QString::number(m_windowLevel));
    }
    if (m_windowWidthLabel)
    {
        m_windowWidthLabel->setText(QString::number(m_windowWidth));
    }
}

void MprViewerWindow::applyWindowPreset(int presetIndex)
{
    if (presetIndex < 0 || presetIndex >= static_cast<int>(kWindowPresets.size()))
    {
        return;
    }

    const WindowPreset& preset = kWindowPresets[static_cast<std::size_t>(presetIndex)];
    {
        const QSignalBlocker blocker(m_windowLevelSlider);
        m_windowLevelSlider->setValue(preset.level);
    }
    {
        const QSignalBlocker blocker(m_windowWidthSlider);
        m_windowWidthSlider->setValue(preset.width);
    }

    m_windowLevel = preset.level;
    m_windowWidth = preset.width;
    updateWindowControlLabels();
    renderAllPanes();
}

VolumeVector3D MprViewerWindow::currentWorldCoordinate() const
{
    if (!m_displayVolume)
    {
        return {};
    }

    const VolumeGeometry& geometry = m_displayVolume->geometry();
    const double localX = static_cast<double>(m_crosshair.x) * geometry.spacing.x;
    const double localY = static_cast<double>(m_crosshair.y) * geometry.spacing.y;
    const double localZ = static_cast<double>(m_crosshair.z) * geometry.spacing.z;

    return {
        geometry.origin.x + (geometry.direction[0] * localX) + (geometry.direction[1] * localY) + (geometry.direction[2] * localZ),
        geometry.origin.y + (geometry.direction[3] * localX) + (geometry.direction[4] * localY) + (geometry.direction[5] * localZ),
        geometry.origin.z + (geometry.direction[6] * localX) + (geometry.direction[7] * localY) + (geometry.direction[8] * localZ)};
}

QPoint MprViewerWindow::crosshairPixel(MprRenderService::Plane plane, const QSize& imageSize) const
{
    const QSize voxelSize = planeVoxelSize(plane);
    if (voxelSize.isEmpty() || imageSize.isEmpty())
    {
        return {};
    }

    auto mapCoordinate = [](int value, int extent, int displayExtent) {
        if (extent <= 1 || displayExtent <= 1)
        {
            return 0;
        }
        const double ratio = static_cast<double>(value) / static_cast<double>(extent - 1);
        return std::clamp(
            static_cast<int>(std::lround(ratio * static_cast<double>(displayExtent - 1))),
            0,
            std::max(0, displayExtent - 1));
    };

    switch (plane)
    {
    case MprRenderService::Plane::Axial:
        return {
            mapCoordinate(m_crosshair.x, voxelSize.width(), imageSize.width()),
            mapCoordinate(m_crosshair.y, voxelSize.height(), imageSize.height())};
    case MprRenderService::Plane::Coronal:
        return {
            mapCoordinate(m_crosshair.x, voxelSize.width(), imageSize.width()),
            mapCoordinate((voxelSize.height() - 1) - m_crosshair.z, voxelSize.height(), imageSize.height())};
    case MprRenderService::Plane::Sagittal:
        return {
            mapCoordinate(m_crosshair.y, voxelSize.width(), imageSize.width()),
            mapCoordinate((voxelSize.height() - 1) - m_crosshair.z, voxelSize.height(), imageSize.height())};
    }

    return {};
}

QRect MprViewerWindow::displayedImageRect(const PaneWidgets& pane) const
{
    if (pane.sourceImageSize.isEmpty() || !pane.imageLabel)
    {
        return {};
    }

    const QSize scaledSize = pane.sourceImageSize.scaled(
        pane.imageLabel->size(),
        Qt::KeepAspectRatio);
    const QPoint topLeft(
        (pane.imageLabel->width() - scaledSize.width()) / 2,
        (pane.imageLabel->height() - scaledSize.height()) / 2);
    return QRect(topLeft, scaledSize);
}

void MprViewerWindow::updateCrosshairFromPanePosition(PaneWidgets& pane, const QPoint& labelPosition)
{
    if (!m_displayVolume)
    {
        return;
    }

    const QRect imageRect = displayedImageRect(pane);
    if (!imageRect.isValid() || !imageRect.contains(labelPosition))
    {
        return;
    }

    const auto& geometry = m_displayVolume->geometry();
    const QSize voxelSize = planeVoxelSize(pane.plane);
    if (voxelSize.isEmpty())
    {
        return;
    }
    const double normalizedX = static_cast<double>(labelPosition.x() - imageRect.left()) /
                               std::max(1, imageRect.width() - 1);
    const double normalizedY = static_cast<double>(labelPosition.y() - imageRect.top()) /
                               std::max(1, imageRect.height() - 1);

    const int planeX = std::clamp(
        static_cast<int>(std::lround(normalizedX * std::max(0, voxelSize.width() - 1))),
        0,
        std::max(0, voxelSize.width() - 1));
    const int planeY = std::clamp(
        static_cast<int>(std::lround(normalizedY * std::max(0, voxelSize.height() - 1))),
        0,
        std::max(0, voxelSize.height() - 1));

    switch (pane.plane)
    {
    case MprRenderService::Plane::Axial:
        m_crosshair.x = std::clamp(planeX, 0, geometry.dimensions.x - 1);
        m_crosshair.y = std::clamp(planeY, 0, geometry.dimensions.y - 1);
        break;
    case MprRenderService::Plane::Coronal:
        m_crosshair.x = std::clamp(planeX, 0, geometry.dimensions.x - 1);
        m_crosshair.z = std::clamp((geometry.dimensions.z - 1) - planeY, 0, geometry.dimensions.z - 1);
        break;
    case MprRenderService::Plane::Sagittal:
        m_crosshair.y = std::clamp(planeX, 0, geometry.dimensions.y - 1);
        m_crosshair.z = std::clamp((geometry.dimensions.z - 1) - planeY, 0, geometry.dimensions.z - 1);
        break;
    }

    renderAllPanes();
}

MprViewerWindow::PaneWidgets* MprViewerWindow::paneForObject(QObject* object)
{
    if (object == m_axialPane.imageLabel)
    {
        return &m_axialPane;
    }
    if (object == m_coronalPane.imageLabel)
    {
        return &m_coronalPane;
    }
    if (object == m_sagittalPane.imageLabel)
    {
        return &m_sagittalPane;
    }

    return nullptr;
}

bool MprViewerWindow::eventFilter(QObject* watched, QEvent* event)
{
    PaneWidgets* pane = paneForObject(watched);
    if (!pane)
    {
        return QMainWindow::eventFilter(watched, event);
    }

    switch (event->type())
    {
    case QEvent::MouseButtonPress:
    {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton)
        {
            m_activeDragPane = pane;
            updateCrosshairFromPanePosition(*pane, mouseEvent->pos());
            return true;
        }
        break;
    }
    case QEvent::MouseMove:
    {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (m_activeDragPane == pane && (mouseEvent->buttons() & Qt::LeftButton))
        {
            updateCrosshairFromPanePosition(*pane, mouseEvent->pos());
            return true;
        }
        break;
    }
    case QEvent::MouseButtonRelease:
    {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton)
        {
            if (m_activeDragPane == pane)
            {
                updateCrosshairFromPanePosition(*pane, mouseEvent->pos());
            }
            m_activeDragPane = nullptr;
            return true;
        }
        break;
    }
    default:
        break;
    }

    return QMainWindow::eventFilter(watched, event);
}

void MprViewerWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    renderAllPanes();
}
