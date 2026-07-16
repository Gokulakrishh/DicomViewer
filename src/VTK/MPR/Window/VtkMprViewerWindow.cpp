#include "VTK/MPR/Window/VtkMprViewerWindow.h"

#include "ViewerTools/WindowLevelPreset.h"
#include "Services/MprMeasurementAnnotationStore.h"
#include "VTK/MPR/MprTypes.h"
#include "VTK/MPR/Window/VtkMprAnnotationDock.h"
#include "VTK/MPR/View/VtkMprView.h"

#include <QAction>
#include <QComboBox>
#include <QDockWidget>
#include <QEvent>
#include <QSignalBlocker>
#include <QToolBar>
#include <cmath>
#include <utility>

namespace
{
constexpr int kDicomWindowPresetBase = 1000;
constexpr int kBuiltInWindowPresetBase = 2000;
}

VtkMprViewerWindow::VtkMprViewerWindow(
    std::shared_ptr<IVolumeData> volume,
    int initialWindowLevel,
    int initialWindowWidth,
    std::vector<DicomWindowPreset> dicomWindowPresets,
    int activeDicomWindowPresetIndex,
    QString seriesInstanceUid,
    DatabaseService* databaseService,
    QWidget* parent)
    : QMainWindow(parent),
      m_dicomWindowPresets(std::move(dicomWindowPresets)),
      m_activeDicomWindowPresetIndex(activeDicomWindowPresetIndex)
{
    if (databaseService)
    {
        m_annotationStore = std::make_unique<MprMeasurementAnnotationStore>(*databaseService);
    }

    m_view = new VtkMprView(
        std::move(volume),
        initialWindowLevel,
        initialWindowWidth,
        std::move(seriesInstanceUid),
        m_annotationStore.get(),
        this);
    setCentralWidget(m_view);
    setupToolbar();
    setupAnnotationDock();
    syncPresetSelection(initialWindowLevel, initialWindowWidth);
    connect(m_view, &VtkMprView::windowLevelWidthChanged, this, &VtkMprViewerWindow::syncPresetSelection);
    resize(1280, 900);
}

VtkMprViewerWindow::~VtkMprViewerWindow() = default;

void VtkMprViewerWindow::changeEvent(QEvent* event)
{
    QMainWindow::changeEvent(event);

    if (event->type() == QEvent::WindowTitleChange && m_view)
    {
        m_view->setContextText(windowTitle());
    }
}

void VtkMprViewerWindow::setupToolbar()
{
    auto* toolbar = addToolBar("MPR Tools");
    toolbar->setMovable(false);

    m_presetComboBox = new QComboBox(toolbar);
    m_presetComboBox->addItem("Custom");

    for (std::size_t index = 0; index < m_dicomWindowPresets.size(); ++index)
    {
        const auto& preset = m_dicomWindowPresets[index];
        const QString explanation = preset.explanation.trimmed();
        const QString prefix = explanation.isEmpty()
            ? QString("DICOM %1").arg(index + 1)
            : QString("DICOM: %1").arg(explanation);
        m_presetComboBox->addItem(
            QString("%1 (%2/%3)")
                .arg(prefix)
                .arg(static_cast<int>(std::lround(preset.center)))
                .arg(static_cast<int>(std::lround(preset.width))),
            comboValueForDicomPresetIndex(static_cast<int>(index)));
    }
    if (!m_dicomWindowPresets.empty())
    {
        m_presetComboBox->insertSeparator(m_presetComboBox->count());
    }

    for (std::size_t index = 0; index < kBuiltInWindowLevelPresets.size(); ++index)
    {
        m_presetComboBox->addItem(
            windowLevelPresetLabel(kBuiltInWindowLevelPresets[index]),
            comboValueForBuiltInPresetIndex(static_cast<int>(index)));
    }
    toolbar->addWidget(m_presetComboBox);
    toolbar->addSeparator();
    m_toolPresentation = std::make_unique<ViewerToolPresentation>(*toolbar, this);
    m_toolPresentation->setSelectionChangedCallback([this](std::optional<ViewerToolId>) {
        applyToolSelection();
    });

    connect(m_presetComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VtkMprViewerWindow::applyWindowPreset);

    applyToolSelection();
}

void VtkMprViewerWindow::setupAnnotationDock()
{
    if (!m_view || !m_annotationStore)
    {
        return;
    }

    m_annotationDockWidget = new QDockWidget("MPR Annotations", this);
    m_annotationDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_annotationDockWidget->setFeatures(
        QDockWidget::DockWidgetMovable |
        QDockWidget::DockWidgetFloatable |
        QDockWidget::DockWidgetClosable);

    m_annotationDock = new VtkMprAnnotationDock(m_annotationDockWidget);
    m_annotationDockWidget->setWidget(m_annotationDock);
    addDockWidget(Qt::RightDockWidgetArea, m_annotationDockWidget);

    connect(m_view, &VtkMprView::mprAnnotationsChanged, m_annotationDock, &VtkMprAnnotationDock::setRecords);
    connect(m_annotationDock, &VtkMprAnnotationDock::goToAnnotationRequested, m_view, &VtkMprView::goToMprAnnotation);
    connect(m_annotationDock, &VtkMprAnnotationDock::deleteAnnotationRequested, m_view, &VtkMprView::deleteMprAnnotation);
    connect(m_annotationDock, &VtkMprAnnotationDock::metadataChanged, m_view, &VtkMprView::updateMprAnnotationMetadata);
    m_annotationDock->setRecords(m_view->mprAnnotationRecords());
}

void VtkMprViewerWindow::applyToolSelection()
{
    if (!m_view)
    {
        return;
    }

    const std::optional<ViewerToolId> activeTool = m_toolPresentation ? m_toolPresentation->activeTool() : std::nullopt;
    MprToolType toolType = MprToolType::Crosshair;
    if (activeTool)
    {
        switch (*activeTool)
        {
        case ViewerToolId::WindowLevel:
            toolType = MprToolType::WindowLevel;
            break;
        case ViewerToolId::Zoom:
            toolType = MprToolType::Zoom;
            break;
        case ViewerToolId::Pan:
            toolType = MprToolType::Pan;
            break;
        case ViewerToolId::Distance:
            toolType = MprToolType::DistanceMeasurement;
            break;
        case ViewerToolId::Polyline:
            toolType = MprToolType::PolylineMeasurement;
            break;
        case ViewerToolId::Angle:
            toolType = MprToolType::AngleMeasurement;
            break;
        case ViewerToolId::RectangleRoi:
            toolType = MprToolType::RectangleRoiMeasurement;
            break;
        }
    }
    m_view->setActiveTool(toolType);
}

void VtkMprViewerWindow::applyWindowPreset(int index)
{
    if (!m_view || index < 0 || !m_presetComboBox)
    {
        return;
    }

    const int value = m_presetComboBox->itemData(index).toInt();
    if (value >= kDicomWindowPresetBase && value < kBuiltInWindowPresetBase)
    {
        const int presetIndex = value - kDicomWindowPresetBase;
        if (presetIndex >= 0 && presetIndex < static_cast<int>(m_dicomWindowPresets.size()))
        {
            const auto& preset = m_dicomWindowPresets[static_cast<std::size_t>(presetIndex)];
            if (preset.width > 0.0)
            {
                m_activeDicomWindowPresetIndex = presetIndex;
                m_view->setWindowLevelWidth(
                    static_cast<int>(std::lround(preset.center)),
                    static_cast<int>(std::lround(preset.width)));
            }
        }
        return;
    }

    if (value >= kBuiltInWindowPresetBase)
    {
        const int presetIndex = value - kBuiltInWindowPresetBase;
        if (presetIndex >= 0 && presetIndex < static_cast<int>(kBuiltInWindowLevelPresets.size()))
        {
            const auto& preset = kBuiltInWindowLevelPresets[static_cast<std::size_t>(presetIndex)];
            m_activeDicomWindowPresetIndex = -1;
            m_view->setWindowLevelWidth(preset.level, preset.width);
        }
    }
}

void VtkMprViewerWindow::syncPresetSelection(int level, int width)
{
    if (!m_presetComboBox)
    {
        return;
    }

    int matchingIndex = 0;
    if (m_activeDicomWindowPresetIndex >= 0 &&
        m_activeDicomWindowPresetIndex < static_cast<int>(m_dicomWindowPresets.size()))
    {
        const int comboIndex = m_presetComboBox->findData(comboValueForDicomPresetIndex(m_activeDicomWindowPresetIndex));
        if (comboIndex >= 0)
        {
            matchingIndex = comboIndex;
        }
    }

    if (matchingIndex == 0)
    {
        for (std::size_t index = 0; index < kBuiltInWindowLevelPresets.size(); ++index)
        {
            const auto& preset = kBuiltInWindowLevelPresets[index];
            if (preset.level == level && preset.width == width)
            {
                const int comboIndex = m_presetComboBox->findData(comboValueForBuiltInPresetIndex(static_cast<int>(index)));
                if (comboIndex >= 0)
                {
                    matchingIndex = comboIndex;
                }
                break;
            }
        }
    }

    const QSignalBlocker blocker(m_presetComboBox);
    m_presetComboBox->setCurrentIndex(matchingIndex);
}

int VtkMprViewerWindow::comboValueForBuiltInPresetIndex(int index) const
{
    return kBuiltInWindowPresetBase + index;
}

int VtkMprViewerWindow::comboValueForDicomPresetIndex(int index) const
{
    return kDicomWindowPresetBase + index;
}
