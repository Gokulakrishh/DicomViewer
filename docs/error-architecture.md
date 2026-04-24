# Error Architecture and Migration Status

## Purpose

This document records the current error-handling architecture so later work can continue without rediscovering the design decisions.

The direction is:

- structured application errors for recoverable workflow/data failures
- exceptions only for invariant violations and internal contract failures
- UI presentation separated from backend error creation
- future audit recording separated from UI presentation

## Current Core Types

The repository now contains the first shared error layer under:

- [AppError.h](/Users/goku/Documents/DicomViewer/src/Errors/AppError.h)
- [AppResult.h](/Users/goku/Documents/DicomViewer/src/Errors/AppResult.h)
- [ErrorCode.h](/Users/goku/Documents/DicomViewer/src/Errors/ErrorCode.h)
- [ErrorSeverity.h](/Users/goku/Documents/DicomViewer/src/Errors/ErrorSeverity.h)
- [IErrorPresenter.h](/Users/goku/Documents/DicomViewer/src/Errors/IErrorPresenter.h)
- [IErrorAudit.h](/Users/goku/Documents/DicomViewer/src/Errors/IErrorAudit.h)

Supporting bridge implementations already exist:

- [NullErrorAudit.h](/Users/goku/Documents/DicomViewer/src/Errors/NullErrorAudit.h)
- [QtErrorDialogPresenter.h](/Users/goku/Documents/DicomViewer/src/Errors/QtErrorDialogPresenter.h)
- [WarningDialogService.h](/Users/goku/Documents/DicomViewer/src/Utilities/WarningDialogService.h)

## Current Design Rules

### Use `AppError` and `AppResult<T>` for

- invalid or incomplete series data
- failed diagnostic-series reload
- inconsistent DICOM geometry
- volume-building failures caused by real-world data
- recoverable 3D preparation failures

### Keep exceptions for

- model invariant violations
- illegal internal state
- programming errors
- lower-level contract failures that should not occur in valid workflows

This split is intentional. It should not be collapsed into “no exceptions anywhere”.

## Current Numeric Error-Code Ranges

The project currently uses grouped numeric codes in [ErrorCode.h](/Users/goku/Documents/DicomViewer/src/Errors/ErrorCode.h):

- `1000` range: common/core application failures
- `3000` range: series loading
- `3100` range: volume building
- `3200` range: volume render preparation
- `3300` range: 3D pipeline
- `3400` range: database
- `3500` range: viewer opening

`ErrorSeverity` remains semantic, not numeric.

## What Is Already Migrated

The following services now return structured results instead of using normal workflow exceptions:

- [SeriesDataLoadService.h](/Users/goku/Documents/DicomViewer/src/Services/SeriesDataLoadService.h)
- [VolumeBuilder.h](/Users/goku/Documents/DicomViewer/src/Services/VolumeBuilder.h)
- [ThreeDSeriesBuildService.h](/Users/goku/Documents/DicomViewer/src/Services/ThreeDSeriesBuildService.h)

The following UI entry points now consume structured errors and present them through the warning/error dialog service:

- [DicomMainWindow.cpp](/Users/goku/Documents/DicomViewer/src/DicomViewerWindow/DicomMainWindow.cpp)
  - `openMprViewer()`
  - `openThreeDViewer()`

The test tools were updated to follow the same contract:

- [ThreeDPipelineHarness.cpp](/Users/goku/Documents/DicomViewer/tests/ThreeDPipelineHarness.cpp)
- [ThreeDSeriesCli.cpp](/Users/goku/Documents/DicomViewer/tests/ThreeDSeriesCli.cpp)

## What Still Intentionally Throws

These areas still use exceptions and should stay that way for now:

- model/container invariants
  - [VolumeData.h](/Users/goku/Documents/DicomViewer/src/Model/VolumeData.h)
  - [MeshData.tpp](/Users/goku/Documents/DicomViewer/src/Model/MeshData.tpp)
  - [SegmentationMaskData.tpp](/Users/goku/Documents/DicomViewer/src/Model/SegmentationMaskData.tpp)
- internal 3D contract failures
  - [ThreeDimensionalPipelineService.cpp](/Users/goku/Documents/DicomViewer/src/Services/ThreeDimensionalPipelineService.cpp)
  - [CompositeMeshPostProcessor.cpp](/Users/goku/Documents/DicomViewer/src/Services/CompositeMeshPostProcessor.cpp)

Those exceptions currently get mapped at the higher `ThreeDSeriesBuildService` boundary when they represent a recoverable top-level 3D build failure.

## Next Error-Architecture Steps

The next concrete work should be:

1. replace string-based exception parsing in [ThreeDSeriesBuildService.cpp](/Users/goku/Documents/DicomViewer/src/Services/ThreeDSeriesBuildService.cpp) with direct structured `AppError` returns from [ThreeDimensionalPipelineService.cpp](/Users/goku/Documents/DicomViewer/src/Services/ThreeDimensionalPipelineService.cpp)
2. introduce a small error-factory layer for domain modules
   - series loading
   - volume building
   - volume rendering
   - 3D pipeline
3. connect `IErrorAudit` to a real audit/log implementation instead of the null implementation
4. decide which failures should be:
   - modal dialog
   - non-blocking warning
   - status-bar only
5. extend the same pattern to future VTK MPR/volume workflows

## Practical Rule for Future Changes

When adding a new service, ask:

- is this a real-world recoverable failure caused by data, configuration, or workflow?
  - use `AppResult<T>`
- is this an internal invariant breach or programming error?
  - keep exception-based failure

That is the rule to preserve.
