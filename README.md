# Cross Axial Dicom Viewer

Cross Axial Dicom Viewer is a `Qt 6` / `C++20` desktop application for browsing and viewing DICOM studies with local SQLite-backed hierarchy indexing, a VTK-based main viewer, VTK-based MPR, annotation workflows, XA cine playback, and DICOM cine/slice-series video export support. The current codebase is a professional-grade desktop viewer foundation aimed at scaling to larger datasets and richer clinical-style workflows.

Current public/demo scope: educational, research, and internal evaluation use only. This software is not currently cleared or approved for diagnosis, treatment, or clinical decision-making.

## Features

- DICOM import with `GDCM`
- metadata-first DICOM folder import using Part-10 DICOM detection
- raw grayscale rendering with DICOM/default `WL/WW`
- slice scrolling and cine playback
- multi-frame XA cine playback
- selected-range XA cine and CT/MR slice-series export to derived non-diagnostic MP4/H.264 video through `GStreamer 1.24.x`
- VTK MPR viewer with synchronized axial / coronal / sagittal panes and a 3D reference pane
- VTK 3D rendering in progress, with QML 3D kept as a transitional surface-view path
- local SQLite metadata, preview, slice, and annotation persistence
- slice-specific distance, angle, and ROI annotation storage
- study browser with hierarchy and representative previews
- crash/application diagnostics for development and tester feedback

## Screenshots

### Main Viewer

![Cross Axial Dicom Viewer main window](docs/screenshots/main-viewer.gif)

### MPR Viewer

![Cross Axial Dicom Viewer MPR viewer](docs/screenshots/mpr-viewer.png)

### 3D Viewer

![Cross Axial Dicom Viewer 3D viewer bone render](docs/screenshots/3D%20Viewer%20-%20Bone.png)

## Clone

```bash
git clone https://github.com/Gokulakrishh/DicomViewer.git
cd DicomViewer
```

## Requirements

You need:

- `CMake >= 3.21`
- `Qt 6`
- `VTK` with Qt 6 support (I built VTK with Qt-6)
- `GDCM`
- `GStreamer 1.24.x` core/App plus reviewed platform plugins:
  - common: `appsrc`, `videoconvert`, `textoverlay`, `h264parse`, `mp4mux`
  - macOS encoder: `vtenc_h264`
  - Windows encoder: `mfh264enc`
- Qt SQL SQLite plugin

Video export is intentionally strict: MP4/H.264 only. There is no OGV or alternative codec fallback.

On macOS with the official GStreamer framework, use:

```bash
export PKG_CONFIG_PATH=/Library/Frameworks/GStreamer.framework/Versions/1.0/lib/pkgconfig
export GST_PLUGIN_SYSTEM_PATH=/Library/Frameworks/GStreamer.framework/Versions/1.0/lib/gstreamer-1.0
```

## Build On A New Machine

### 1. Local SQLite

The default desktop build uses local SQLite. No database server, database username, or database password is required.
The database file is created automatically under:

`Documents/Cross Axial Dicom Viewer/data/crossaxial.sqlite`

### 2. Build

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x
cmake --build build -j4
```

For a VTK-enabled local macOS build, pass your Qt and Qt6-built VTK locations:

```bash
cmake -S . -B build-vtk \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x \
  -DVTK_DIR=/path/to/vtk/lib/cmake/vtk-9.5

cmake --build build-vtk --target CrossAxialDicomViewer --parallel 4
```

## First Run

After launch:

1. Open a DICOM folder.
2. Let the app import metadata into the local SQLite database.
3. Browse patients, studies, and series from the left tree.
4. Select a series to load it into the VTK main viewer.
5. Use the toolbar for zoom/pan, WL/WW, measurement, annotation, cine playback, and cine/slice-series export.
6. Use `View -> Open MPR` for VTK-based multi-planar reconstruction on multi-slice series.
7. Use `View -> 3D` for 3D reconstruction workflows.

## Video Export

Video export creates a derived, non-diagnostic MP4/H.264 file from the selected multi-frame cine object or CT/MR slice series. The export path:

- uses the current DICOM cine timing when available
- uses a controlled manual FPS default for slice-series exports when no DICOM cine timing exists
- applies the current WL/WW rendering
- excludes patient-identifying overlays from the generated video
- adds a non-diagnostic watermark
- writes only MP4/H.264 output
- cancels cleanly and removes partial files on failure
