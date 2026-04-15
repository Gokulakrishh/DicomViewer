# DicomViewer

`DicomViewer` is a `Qt 6` / `C++20` desktop application for loading, browsing, and viewing DICOM studies with PostgreSQL-backed hierarchy indexing.

## Overview

Current capabilities:
- DICOM import with `GDCM`
- PostgreSQL persistence for `Patient -> Study -> Series -> DicomImage`
- lazy hierarchy loading for `Patient -> Study -> Series`
- one cached preview image per series stored in the database
- raw-slice DICOM viewing with true grayscale `WL/WW`
- slice browsing with slider, mouse wheel, and cine playback
- measurement tools:
  - distance
  - pixel probe / HU probe
  - angle

### Domain
- `Patient`
- `Study`
- `Series`
- `DicomImage`

### Loading / Persistence
- `GDCMFileHandling`
  - scans folders
  - loads DICOM tags and raw pixel data
- `PostgreService`
  - stores and retrieves patient/study/series/image hierarchy
  - stores one preview PNG per series for fast tree/preview reload

### Viewer
- `ViewportSession`
  - per-viewport state container
  - current series
  - current slice index
  - WL / WW
  - preset
  - tool
  - cine state
- `DicomViewportController`
  - operates on one `ViewportSession`
  - manages current slice selection
  - manages preload scheduling
  - manages window/preset state
- `DicomRenderService`
  - converts raw grayscale slice data into a display pixmap
  - applies true DICOM window level / width behavior
- `MeasurementController`
  - computes distance, probe, and angle results
- `DicomGraphicsView`
  - input handling
  - scene overlays
  - image presentation
- `DicomMainWindow`
  - UI composition and wiring

## Data Flow

### Folder import
```text
Folder
  -> GDCMFileHandling
  -> Patient / Study / Series / DicomImage hierarchy
  -> PostgreService
  -> PostgreSQL tables + preview PNG cache
  -> background import progress
  -> tree refresh
```

### Display
```text
Tree selection
  -> PostgreService::getSeries(...)
  -> DicomViewportController
  -> raw DICOM load if needed
  -> DicomRenderService
  -> DicomGraphicsView
```

### Tools
```text
Mouse interaction
  -> DicomGraphicsView
  -> DicomMainWindow
  -> MeasurementController
  -> overlay drawn back in DicomGraphicsView
```

## Build Requirements

- `CMake >= 3.21`
- `C++20`
- `Qt 6`
  - `Core`
  - `Gui`
  - `Widgets`
  - `Sql`
  - `Concurrent`
- `GDCM`
- `OpenCV >= 4.6`
- PostgreSQL client libraries and Qt `QPSQL` plugin for runtime DB access

## Local Build

```bash
cmake -S . -B build
cmake --build build -j4
```

If Qt is in a custom location:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x
cmake --build build -j4
```

## Runtime Configuration

Runtime settings are read from `config.ini`.

Example:

```ini
[database]
hostName=127.0.0.1
port=5432
databaseName=dicomviewer
userName=postgres
password=your_password
```

`config.ini` contains the non-secret AI settings:

```ini
[ai]
provider=gemini
baseUrl=https://generativelanguage.googleapis.com
apiKey=
model=gemini-2.5-flash
defaultReasoningLevel=medium
requestTimeoutMs=30000
maxOutputTokens=2048
```

The `apiKey` value is stored directly in `config.ini`.
Environment variables such as `DICOMVIEWER_AI_APIKEY` still override the file when needed.

The app stores:
- hierarchy metadata in PostgreSQL
- preview PNG blobs in `series`
- original DICOM file path for raw reloading

The raw DICOM file remains the source of truth for full-fidelity rendering.

## CI

GitHub Actions CI is defined in:
- `.github/workflows/ci.yml`

It currently:
- installs Linux build dependencies
- configures with CMake + Ninja
- builds the project

## Roadmap

Planned expansion areas:
- MPR
- 3D volume rendering
- richer ROI tools
- persisted annotations / measurements
- multi-viewport workflows

## Next Stage Notes

The current architecture is a solid scalable baseline for local browsing and viewing. The next stage is optimization and polish rather than another architectural rewrite.

Planned next-stage work:
- add a preview cache service so repeated patient/study clicks do not re-query and re-decode the same series thumbnail
- move series loading to an async path for very large studies
- add cancellation support for long-running import and search operations
- add cache eviction policies for raw series buffers and derived MPR/volume data
