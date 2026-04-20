# VTK and Future ITK Architecture

## Purpose

This document defines the long-term rendering and algorithm direction for `DicomViewer`.

Current state:
- Qt drives the workstation shell and workflow.
- QML-based 3D is a temporary surface-view layer.
- The backend already separates:
  - lightweight browsing data
  - diagnostic series loading
  - volume building
  - 3D profile selection
  - segmentation / mesh extraction / post-processing

Target state:
- `VTK` becomes the primary 3D rendering engine for medical visualization.
- `ITK` remains a future algorithm space for segmentation, filtering, and label-volume workflows.
- Qt stays the application shell, not the core medical rendering engine.

## Why This Direction

`VTK` is the better long-term rendering choice for:
- direct volume rendering
- transfer functions
- clipping planes
- medical-style 3D interaction
- surface and volume rendering in one stack

`ITK` is the better future choice for:
- segmentation
- morphology
- filtering
- organ labeling
- registration and other medical image analysis workflows

This split is closer to established medical-imaging practice than extending Qt Quick 3D indefinitely.

## Layered Architecture

### 1. Qt Application Layer

Responsibilities:
- menus
- docking
- workflow orchestration
- preferences
- status / progress / errors

Key rule:
- UI code must not own image-processing or rendering algorithms.

### 2. Domain and Data Layer

Current core services already fit this role:
- `SeriesDataLoadService`
- `VolumeBuilder`
- `VolumeResampleService`
- `ThreeDimensionalPipelineService`
- `ThreeDSeriesBuildService`

Responsibilities:
- load diagnostic series
- convert slices into a coherent volume
- expose renderer-ready or algorithm-ready data structures

Key rule:
- this layer remains renderer-agnostic.

### 3. Rendering Adapter Layer

This is the seam between domain data and VTK objects.

Planned adapters:
- `VtkVolumeAdapter`
- `VtkMeshAdapter`
- later `VtkLabelMapAdapter`

Responsibilities:
- convert `IVolumeData` to `vtkImageData`
- convert `IMeshData` to `vtkPolyData`
- convert future label volumes into VTK-compatible representations

Key rule:
- VTK-specific types stay here, not in the domain models.

### 4. Rendering Module Layer

Planned modules:
- `VTKVolumeViewerWindow`
- later `VTKSurfaceViewerWindow`
- later a shared `VTKRenderingController`

Responsibilities:
- volume rendering presets
- transfer-function updates
- camera control
- clipping
- lighting
- scene composition

Key rule:
- this layer renders domain results but does not implement segmentation logic.

### 5. Future ITK Algorithm Layer

Planned future space:
- `ITKSegmentationStrategy`
- `ITKMorphologyRefinementStrategy`
- `ITKLabelVolumeBuilder`

Responsibilities:
- algorithm implementations behind existing interfaces
- binary mask refinement
- organ label generation
- later multi-label workflows

Key rule:
- ITK remains an implementation option behind strategy interfaces, not a UI dependency.

## Immediate Rendering Strategy

### Temporary

Keep the current QML 3D viewer as:
- a temporary surface-view mode
- a development tool for mesh inspection

### Long-term

Move `View -> 3D` toward:
- `VTKVolumeViewerWindow` for direct volume rendering

Later add:
- `View -> 3D Surface` for mesh/surface rendering
- `View -> 3D Volume` for direct volume rendering

This avoids forcing one rendering mode to solve every problem.

## Current Integration Constraint

The repository now contains the first VTK volume-viewer scaffolding:
- `src/AdvancedViewer/VtkVolumeAdapter.*`
- `src/AdvancedViewer/VtkVolumeViewerWindow.*`

Important constraint on this machine:
- the available VTK with `GUISupportQt` is built against `Qt5`
- the application is built with `Qt6`

Consequence:
- VTK integration is gated behind `DICOMVIEWER_ENABLE_VTK`
- the default build keeps the QML viewer active until a Qt6-compatible VTK build is available

This is intentional. Mixing `Qt5` and `Qt6` in the same viewer process is the wrong foundation.

## Recommended Next Migration Steps

### Phase 1

- keep QML 3D temporary
- keep backend pipeline work renderer-agnostic
- land VTK viewer scaffolding behind a build flag

### Phase 2

When a Qt6-compatible VTK build is available:
- enable `DICOMVIEWER_ENABLE_VTK`
- route `View -> 3D` to `VTKVolumeViewerWindow`
- validate direct volume rendering on real CT datasets

### Phase 3

Add:
- `VtkMeshAdapter`
- `VTKSurfaceViewerWindow`

This preserves the current segmentation/surface pipeline while allowing VTK to render it properly.

### Phase 4

Introduce future ITK-backed algorithms behind existing interfaces:
- segmentation strategies
- morphology refinement
- organ-label generation

## SOLID Guidance

### Single Responsibility

- domain services load and transform data
- adapters convert data for VTK
- viewer windows render
- profiles choose anatomy-specific behavior

### Open/Closed

New rendering modes should be added by:
- new adapters
- new viewer modules
- new profiles

not by modifying core domain models to know about VTK or ITK.

### Liskov / Interface Stability

Keep current abstractions stable:
- `IVolumeData`
- `IMeshData`
- `ISegmentationStrategy`
- `IMeshExtractionStrategy`
- `IMeshPostProcessor`

Future ITK or VTK-backed implementations should conform to these, not bypass them.

### Interface Segregation

Avoid one giant rendering interface.

Prefer focused seams such as:
- `IVolumeRenderingFacade`
- `ISurfaceRenderingFacade`
- `IVolumeDataAdapter`
- `IMeshDataAdapter`

### Dependency Inversion

UI depends on:
- launchers
- controllers
- facades

not directly on VTK pipelines or ITK filters.

## Long-Term Goal

The target stack is:
- `Qt` for application shell
- `VTK` for rendering
- `ITK` for medical image processing
- project-owned service interfaces and data models in the middle

That is the direction most compatible with long-term growth, maintainability, and future regulated-product discipline.
