# Current Memory Issues

This document records only the memory problems that still exist in the current codebase.

Resolved items are intentionally omitted.

## Current Measured State

Measured on the `VTK` build while one study was open in MPR:

- process: [DicomViewer.app](/Users/goku/Documents/DicomViewer/build/DicomViewer.app)
- resident memory `RSS`: about `855,652 KB` (`~836 MiB`)
- virtual memory `VSZ`: about `36,117,420 KB` (`~34.4 GiB`)

`RSS` is the metric that matters for RAM pressure.

## Remaining Problems

### 1. Slice objects and built volume coexist

The app still keeps both:

- slice-level `DicomImage` objects with raw pixels
- a contiguous `VolumeData<int16_t>` built for MPR / 3D

This is not inherently wrong, but it is still the main source of duplicate image ownership.

Relevant code:

- [DicomViewportController.cpp](/Users/goku/Documents/DicomViewer/src/DicomViewerWindow/DicomViewportController.cpp)
- [DicomMainWindow.cpp](/Users/goku/Documents/DicomViewer/src/DicomViewerWindow/DicomMainWindow.cpp)
- [VolumeBuilder.cpp](/Users/goku/Documents/DicomViewer/src/Services/VolumeBuilder.cpp)

### 2. `VolumeBuilder` creates a full contiguous volume buffer

This is still a full additional copy of the loaded slice data:

- [VolumeBuilder.cpp](/Users/goku/Documents/DicomViewer/src/Services/VolumeBuilder.cpp)
- [VolumeData.h](/Users/goku/Documents/DicomViewer/src/Model/VolumeData.h)

This buffer is justified and should remain the canonical 3D representation for MPR / 3D.

It is still a significant memory consumer.

### 3. 3D preprocessing creates another derived full volume

The 3D viewer preprocessing path still creates:

- `baseVolume`
- `boneFocusedVolume`

where `boneFocusedVolume` is a second full `VolumeData<int16_t>`:

- [VolumeRenderPreprocessingService.cpp](/Users/goku/Documents/DicomViewer/src/Services/VolumeRenderPreprocessingService.cpp)

This is still a meaningful full-volume memory cost.

### 4. Main 2D VTK view still copies the current slice into `vtkImageData`

The main viewer is not using `QPixmap` anymore for primary display, but it still creates a VTK copy of the current slice:

- [VtkSliceSceneAdapter.cpp](/Users/goku/Documents/DicomViewer/src/VTK/MainView/VtkSliceSceneAdapter.cpp)

This is only one slice at a time, so it is not the largest memory problem, but it still exists.

### 5. Series preview thumbnails are still retained

This is now an intentional and acceptable cost, but it still exists:

- one preview thumbnail per `Series`

Relevant code:

- [DicomParameters.h](/Users/goku/Documents/DicomViewer/src/Model/DicomParameters.h)
- [PostgreService.cpp](/Users/goku/Documents/DicomViewer/src/Database/PostgreService.cpp)
- [DicomTreePanel.cpp](/Users/goku/Documents/DicomViewer/src/DicomViewerWindow/DicomTreePanel.cpp)

These should stay thumbnail-sized and should not grow back into per-slice diagnostic pixmap retention.

### 6. General Qt / VTK / OpenGL overhead

The app still pays for:

- Qt object overhead
- VTK renderers and pipelines
- OpenGL render windows and driver-side resources
- allocator retention / fragmentation

This is real but secondary.

## What Is Already Resolved

These are no longer active problems:

- `DicomImage` raw storage as `QVector<int>`
- wasteful per-slice DICOM pixmap retention for the normal monochrome DICOM path
- deep VTK full-volume copy for the standard `VolumeData<int16_t>` import path

## Current Priority Order

If memory work continues, the next priorities should be:

1. reduce simultaneous lifetime of slice raw buffers and built volume buffers
2. review whether the derived `boneFocusedVolume` can be made cheaper or more lazy
3. decide whether slice raw buffers can be evicted after volume build when safe
4. add explicit instrumentation for:
   - loaded slice count
   - slice raw bytes
   - volume bytes
   - derived 3D volume bytes
   - VTK image bytes
