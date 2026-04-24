#pragma once

#include <QtGlobal>

class DicomViewportController;
class VtkDiagnosticSliceView;

namespace MemoryManagementDebug
{
void logMainViewerSnapshot(
    const DicomViewportController* viewportController,
    const VtkDiagnosticSliceView* view,
    const char* reason);
}
