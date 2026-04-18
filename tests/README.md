# Tests

This folder contains development-side validation tools for the 3D pipeline.

These are not part of the production app UI.

## Available executables

### `ThreeDPipelineHarness`

Purpose:

- validates the 3D backend on synthetic data
- checks:
  - direct volume -> pipeline
  - synthetic series -> `ThreeDSeriesBuildService` -> `VolumeBuilder` -> pipeline

Build:

```bash
cmake --build build -j4 --target ThreeDPipelineHarness
```

Run:

```bash
./build/ThreeDPipelineHarness
```

Expected output includes:

- profile name
- foreground voxel count
- mesh vertex count
- mesh triangle count

### `ThreeDSeriesCli`

Purpose:

- runs the real 3D backend on an actual DICOM folder
- uses production code:
  - `GDCMFileHandling`
  - `ThreeDSeriesBuildService`
  - `VolumeBuilder`
  - `ThreeDimensionalPipelineService`

Build:

```bash
cmake --build build -j4 --target ThreeDSeriesCli
```

Run:

```bash
./build/ThreeDSeriesCli --folder /path/to/dicom/folder --profile bone
```

Supported profiles:

- `bone`
- `lung`

Optional series selection:

```bash
./build/ThreeDSeriesCli --folder /path/to/dicom/folder --profile lung --series-uid YOUR_SERIES_UID
./build/ThreeDSeriesCli --folder /path/to/dicom/folder --profile bone --series-number 3
```

Behavior:

- if `--series-uid` is given, that series is selected
- else if `--series-number` is given, the first matching series number is selected
- else the first discovered series is used

Expected output includes:

- selected patient/study/series information
- series image count
- foreground voxel count
- mesh vertex count
- mesh triangle count

## Notes

- use these tools to validate backend behavior before working on QML geometry/rendering
- if backend counts or thresholds look wrong here, fix backend logic first
- `VolumeBuilder` debug logs are currently still enabled intentionally for 3D validation
