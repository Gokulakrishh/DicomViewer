# Current Memory Issues

This document records only the memory problems that still exist in the current codebase.

Resolved items are intentionally omitted.

## Current Measured State

Measured on the `VTK` build after the recent memory cleanup:

- main viewer only, bounded raw-slice cache active:
  - resident memory `RSS`: about `269,012 KB` (`~262.7 MiB`)
  - debug snapshot example:
    - `loadedSlices=20`
    - `rawSliceBytes=10.00 MiB`
    - `previewCount=1`
    - `previewBytes=1.00 MiB`
    - `vtkSliceBytes=512.00 KiB`

- one study open in MPR:
  - resident memory `RSS`: about `461,056 KB` (`~450.3 MiB`)

`RSS` is the metric that matters for RAM pressure.

## Remaining Problems

### 1. Slice objects and built volume can still coexist

The app still keeps both:

- slice-level `DicomImage` objects with raw pixels in the main viewer cache
- a contiguous `VolumeData<int16_t>` built for MPR / 3D

This is reduced compared with the old design because:

- the main viewer now uses a bounded raw-slice cache
- MPR / 3D now load through `AdvancedSeriesVolumeService`

It is still a real duplicate-memory condition while advanced viewers are open.

Relevant code:

- [DicomViewportController.cpp](/Users/goku/Documents/DicomViewer/src/DicomViewerWindow/DicomViewportController.cpp)
- [AdvancedSeriesVolumeService.cpp](/Users/goku/Documents/DicomViewer/src/Services/AdvancedSeriesVolumeService.cpp)
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
- main-window forced full-series raw loading for MPR launch

## Current Priority Order

If memory work continues, the next priorities should be:

1. reduce simultaneous lifetime of slice raw buffers and built volume buffers
2. review whether the derived `boneFocusedVolume` can be made cheaper or more lazy
3. evaluate whether the current-slice VTK copy in the main 2D viewer is worth optimizing
4. add explicit instrumentation for:
   - loaded slice count
   - slice raw bytes
   - volume bytes
   - derived 3D volume bytes
   - VTK image bytes
