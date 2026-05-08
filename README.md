# Cross Axial Dicom Viewer

Cross Axial Dicom Viewer is a `Qt 6` / `C++20` desktop application for browsing and viewing DICOM studies with local SQLite-backed hierarchy indexing, a VTK-based main viewer, VTK-based MPR, and annotation workflows. The current codebase is a professional-grade desktop viewer foundation aimed at scaling to larger datasets and richer clinical-style workflows.

## Features

- DICOM import with `GDCM`
- raw grayscale rendering with true `WL/WW`
- slice scrolling and cine playback
- VTK MPR viewer with synchronized axial / coronal / sagittal panes and a 3D reference pane
- VTK 3D rendered (on progress)
- local SQLite metadata and annotation persistence

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
- Qt SQL SQLite plugin

## Build On A New Machine

### 1. Configure Local SQLite

The default desktop build uses local SQLite. No database server, database username, or database password is required.

Example `config.ini`:

```ini
[database]
databaseName=dicomviewer.sqlite

[ai]
provider=none
baseUrl=
apiKey=
```

### 2. Build

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x
cmake --build build -j4
```

## First Run

After launch:

1. Open a DICOM folder.
2. Let the app import metadata into the local SQLite database.
3. Browse patients, studies, and series from the left tree.
4. Select a series to load it into the VTK main viewer.
5. Use `View -> Open MPR` for VTK-based multi-planar reconstruction on multi-slice series.
5. Use `View -> 3D` for VTK-based 3D reconstruction of bone or lung series.
