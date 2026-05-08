#pragma once

#include <QtGlobal>

class DicomViewportController;
class VtkDiagnosticSliceView;

namespace MemoryManagementDebug
{
/**
 * @brief Logs a lightweight memory/debug snapshot for the main viewer.
 * @param viewportController Viewport controller to inspect.
 * @param view Diagnostic slice view to inspect.
 * @param reason Short reason/context for the snapshot.
 */
void logMainViewerSnapshot(
    const DicomViewportController* viewportController,
    const VtkDiagnosticSliceView* view,
    const char* reason);
}
