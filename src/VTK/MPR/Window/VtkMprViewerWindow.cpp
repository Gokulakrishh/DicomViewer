#include "VTK/MPR/Window/VtkMprViewerWindow.h"

#include "VTK/MPR/MprTypes.h"
#include "VTK/MPR/View/VtkMprView.h"

#include <QAction>
#include <QComboBox>
#include <QEvent>
#include <QSignalBlocker>
#include <QToolBar>
#include <array>
#include <utility>

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
    {"Soft Tissue", 50, 350},
    {"Bone", 400, 1800},
    {"Lung", -600, 1400},
}};
}

VtkMprViewerWindow::VtkMprViewerWindow(
    std::shared_ptr<IVolumeData> volume,
    int initialWindowLevel,
    int initialWindowWidth,
    QWidget* parent)
    : QMainWindow(parent)
{
    m_view = new VtkMprView(std::move(volume), initialWindowLevel, initialWindowWidth, this);
    setCentralWidget(m_view);
    setupToolbar();
    syncPresetSelection(m_view->currentWindowLevel(), m_view->currentWindowWidth());
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
    for (const auto& preset : kWindowPresets)
    {
        m_presetComboBox->addItem(QString::fromLatin1(preset.name));
    }
    toolbar->addWidget(m_presetComboBox);
    toolbar->addSeparator();

    m_windowLevelAction = toolbar->addAction("WL/WW");
    m_zoomAction = toolbar->addAction("Zoom");

    m_windowLevelAction->setCheckable(true);
    m_zoomAction->setCheckable(true);

    connect(m_presetComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VtkMprViewerWindow::applyWindowPreset);

    connect(m_windowLevelAction, &QAction::toggled, this, [this](bool checked) {
        if (checked)
        {
            setExclusiveToolAction(m_windowLevelAction, MprToolType::WindowLevel);
        }
        else if (!m_zoomAction->isChecked())
        {
            m_view->setActiveTool(MprToolType::Crosshair);
        }
    });
    connect(m_zoomAction, &QAction::toggled, this, [this](bool checked) {
        if (checked)
        {
            setExclusiveToolAction(m_zoomAction, MprToolType::Zoom);
        }
        else if (!m_windowLevelAction->isChecked())
        {
            m_view->setActiveTool(MprToolType::Crosshair);
        }
    });
}

void VtkMprViewerWindow::setExclusiveToolAction(QAction* activeAction, MprToolType toolType)
{
    if (activeAction != m_windowLevelAction)
    {
        m_windowLevelAction->blockSignals(true);
        m_windowLevelAction->setChecked(false);
        m_windowLevelAction->blockSignals(false);
    }
    if (activeAction != m_zoomAction)
    {
        m_zoomAction->blockSignals(true);
        m_zoomAction->setChecked(false);
        m_zoomAction->blockSignals(false);
    }

    m_view->setActiveTool(toolType);
}

void VtkMprViewerWindow::applyWindowPreset(int index)
{
    if (!m_view || index <= 0)
    {
        return;
    }

    const auto& preset = kWindowPresets[static_cast<std::size_t>(index - 1)];
    m_view->setWindowLevelWidth(preset.level, preset.width);
}

void VtkMprViewerWindow::syncPresetSelection(int level, int width)
{
    if (!m_presetComboBox)
    {
        return;
    }

    int matchingIndex = 0;
    for (std::size_t index = 0; index < kWindowPresets.size(); ++index)
    {
        const auto& preset = kWindowPresets[index];
        if (preset.level == level && preset.width == width)
        {
            matchingIndex = static_cast<int>(index) + 1;
            break;
        }
    }

    const QSignalBlocker blocker(m_presetComboBox);
    m_presetComboBox->setCurrentIndex(matchingIndex);
}
