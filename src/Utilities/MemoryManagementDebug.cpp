#include "Utilities/MemoryManagementDebug.h"

#include "DicomViewerWindow/DicomViewportController.h"
#include "Model/DicomImage.h"
#include "Model/DicomParameters.h"
#include "VTK/MainView/VtkDiagnosticSliceView.h"

#include <QLoggingCategory>
#include <QString>

Q_LOGGING_CATEGORY(lcMemoryManagement, "dicomviewer.memory.management")

namespace
{
QString formatBytes(qint64 bytes)
{
    constexpr qint64 kib = 1024;
    constexpr qint64 mib = 1024 * kib;

    if (bytes >= mib)
    {
        return QString("%1 MiB").arg(QString::number(static_cast<double>(bytes) / static_cast<double>(mib), 'f', 2));
    }
    if (bytes >= kib)
    {
        return QString("%1 KiB").arg(QString::number(static_cast<double>(bytes) / static_cast<double>(kib), 'f', 2));
    }
    return QString("%1 B").arg(bytes);
}
}

void MemoryManagementDebug::logMainViewerSnapshot(
    const DicomViewportController* viewportController,
    const VtkDiagnosticSliceView* view,
    const char* reason)
{
#if DICOMVIEWER_ENABLE_MEMORY_LOGGING
    qint64 loadedSliceCount = 0;
    qint64 rawSliceBytes = 0;
    qint64 previewBytes = 0;
    int previewCount = 0;

    if (viewportController)
    {
        if (const auto series = viewportController->currentSeries())
        {
            if (!series->previewPixmap().isNull())
            {
                previewCount = 1;
                previewBytes = static_cast<qint64>(series->previewPixmap().width()) *
                               static_cast<qint64>(series->previewPixmap().height()) *
                               static_cast<qint64>(series->previewPixmap().depth()) / 8;
            }

            for (const auto& image : series->images())
            {
                if (!image || !image->hasRawPixels())
                {
                    continue;
                }

                ++loadedSliceCount;
                rawSliceBytes += static_cast<qint64>(image->rawPixelByteCount());
            }
        }
        else if (const auto* image = viewportController->currentImage(); image && image->hasRawPixels())
        {
            loadedSliceCount = 1;
            rawSliceBytes = static_cast<qint64>(image->rawPixelByteCount());
        }
    }

    const qint64 vtkSliceBytes = view ? view->currentImageByteCount() : 0;
    qCDebug(lcMemoryManagement).noquote()
        << "[main-view]"
        << reason
        << "| loadedSlices=" << loadedSliceCount
        << "| rawSliceBytes=" << formatBytes(rawSliceBytes)
        << "| previewCount=" << previewCount
        << "| previewBytes=" << formatBytes(previewBytes)
        << "| vtkSliceBytes=" << formatBytes(vtkSliceBytes);
#else
    Q_UNUSED(viewportController);
    Q_UNUSED(view);
    Q_UNUSED(reason);
#endif
}
