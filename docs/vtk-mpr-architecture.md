# VTK MPR Architecture

## Purpose

This document defines the clean restart architecture for a future `VTK`-based MPR implementation in `DicomViewer`.

Current repository state:
- VTK is retained only for 3D volume rendering.
- MPR currently uses the existing non-VTK viewer path.
- the previous VTK MPR implementation was removed and should not be revived

This document is the source of truth for the next attempt.

## Scope

This architecture covers:
- axial / coronal / sagittal MPR
- shared anatomical cursor state in world space
- tool isolation
- future 4-up layout with a 3D reference pane

This architecture does not cover:
- segmentation workflows
- full diagnostic slice-view replacement
- organ labeling
- advanced measurement workflows in the first phase

## Core Principle

The authoritative state in MPR is:
- one shared anatomical cursor position in world space

Everything else is derived from that:
- axial slice index
- coronal slice index
- sagittal slice index
- in-plane tracking overlays
- future 3D reference alignment

This means:
- sliders do not directly command multiple panes
- tools do not directly command individual viewers
- views do not own synchronization logic

## High-Level Model

### Slice change flow

- User drags axial slider
- `MprController.setSlice(Axial, value)`
- controller maps the new axial slider value to updated world-space `CursorPosition`
- `MprScene` updates `CursorPosition`
- `MprScene` emits state change
- all pane views receive update
- each pane derives its own reslice state from `CursorPosition`
- VTK renders

### Tool interaction flow

- Mouse drag
- active tool interprets the gesture
- tool updates `CursorPosition` or `WindowLevelState`
- `MprScene` emits state change
- views update automatically

This is the required interaction model.

## Architecture Diagram

- `Application`
- `MprScene`
- `Controllers`
- `Pane Views`
- `VTK Adapters`
- `VTK Rendering Pipeline`

The practical flow is:

- `Window -> View -> InteractionRouter -> ActiveTool -> Controller -> Scene -> Views -> VTK Adapter -> VTK`

The important point is:
- input always becomes scene state updates
- rendering always becomes a reaction to scene state

## Module Layout

Planned code layout:

- `src/VTK/MPR/Window`
- `src/VTK/MPR/View`
- `src/VTK/MPR/State`
- `src/VTK/MPR/Controllers`
- `src/VTK/MPR/Sync`
- `src/VTK/MPR/Adapters`
- `src/VTK/MPR/Tools`

Recommended primary classes:

- `VtkMprViewerWindow`
- `VtkMprView`
- `IMprPaneView`
- `VtkSliceMprPaneView`
- `VtkThreeDReferencePaneView`
- `MprScene`
- `MprController`
- `ToolController`
- `InteractionRouter`
- `MprSynchronizationService`
- `VtkMprSceneAdapter`
- `VtkMprToolAdapter`

## State Design

### `MprScene`

`MprScene` is the application-facing MPR state model.

It should contain only authoritative shared state:
- `CursorPosition` in world space
- `WindowLevelState`
- `ActiveToolState`
- later explicit mode flags if needed

It should not become a large storage bucket for pane-local transient values.

### Derived state

The following are derived, not authoritative:
- axial slice index
- coronal slice index
- sagittal slice index
- per-pane overlay positions
- future 3D reference plane positions

Derived state should be recomputed from scene state through synchronization services or adapters.

## Responsibilities

### `VtkMprViewerWindow`

Responsibilities:
- top-level layout
- toolbar
- status widgets
- lifecycle behavior

Must not own:
- synchronization logic
- VTK scene setup
- direct tool execution

### `VtkMprView`

Responsibilities:
- composite container for pane views
- event capture
- forwarding normalized interaction to the routing layer
- view refresh coordination

Must not own:
- business rules
- anatomical synchronization policy
- domain data loading

### `IMprPaneView`

Responsibilities:
- one pane abstraction
- render surface exposure
- overlay surface exposure if needed
- apply pane-specific derived state

Concrete implementations:
- `VtkSliceMprPaneView`
- later `VtkThreeDReferencePaneView`

### `MprController`

Responsibilities:
- convert UI intents into scene-state changes
- map slider movement to world-space cursor changes
- coordinate controller-level commands such as reset

Example:
- `setSlice(Axial, value)` does not mean “set three pane slices”
- it means “update the world-space cursor so axial corresponds to this value”

### `ToolController`

Responsibilities:
- track the currently active tool
- guarantee one active tool at a time

It should not interpret mouse movement itself.

### `InteractionRouter`

Responsibilities:
- receive normalized input events from the view
- route them to the active tool only
- ensure there is exactly one input path for the current tool

### `MprSynchronizationService`

Responsibilities:
- derive pane-specific state from scene state
- map world-space cursor position into pane-specific slice/reslice state
- derive overlay positions from the shared cursor

This is where synchronization rules live.

Not:
- in the window
- in the pane widgets
- in toolbar code

### `VtkMprSceneAdapter`

Responsibilities:
- bind derived MPR state to `vtkResliceImageViewer`
- create and own VTK viewer objects
- apply orientation
- apply camera state
- trigger renders

This is the only layer that should know how MPR state is translated into VTK operations.

### `VtkMprToolAdapter`

Responsibilities:
- execute VTK-specific operations needed by active tools
- expose minimal VTK-native interaction capabilities behind one interface

Examples:
- enable reslice cursor widget for `Crosshair`
- apply WL/WW deltas
- apply pan/zoom camera changes
- later enable distance / angle widgets

## Tool Model

Planned tools:
- `Crosshair`
- `WL/WW`
- `Pan`
- `Zoom`
- `Slice`
- later `Distance`
- later `Angle`

### Tool rules

Required rules:
- one active tool at a time
- one input path per tool
- no fallback to default VTK behavior

### Recommended split

App-owned tools:
- `WL/WW`
- `Pan`
- `Zoom`
- `Slice`

VTK-native tools:
- `Crosshair`
- later `Distance`
- later `Angle`

If a tool uses the VTK-native path, that path must be the only enabled interaction route while the tool is active.

If a tool uses the app-owned path, events must be consumed at the app layer and must not fall through to VTK.

## Input Architecture

Required flow:

- `Window -> View -> InteractionRouter -> ActiveTool -> Controller/ToolAdapter -> Scene -> Synchronization -> Pane Views -> VTK Adapter`

Two valid patterns exist:

### 1. Tool updates scene state directly

- tool interprets gesture
- tool updates `MprScene`
- synchronization layer derives pane state
- VTK adapter applies result

### 2. Tool delegates through controller

- tool interprets gesture
- tool asks `MprController` to update scene
- controller updates `MprScene`
- synchronization layer derives pane state
- VTK adapter applies result

Either is acceptable.

The non-negotiable rule is:
- tool interaction must end in shared scene-state updates

## SOLID Guidance

### Single Responsibility

- `Window` owns shell UI
- `View` owns event capture and pane composition
- `Controller` owns intent-to-state translation
- `Scene` owns authoritative state
- `SynchronizationService` owns derived pane-state logic
- `Adapter` owns VTK translation
- `ToolController` owns active tool selection

Each of these should have one reason to change.

### Open/Closed

New tools, new pane types, or future 3D reference behavior should be added by:
- new tool classes
- new pane implementations
- new synchronization rules

Do not keep rewriting the same window class for every feature.

### Liskov Substitution

Pane abstractions must remain substitutable:
- `IMprPaneView`
- `VtkSliceMprPaneView`
- future `VtkThreeDReferencePaneView`

The window and synchronization layer must work against pane abstractions, not concrete assumptions.

### Interface Segregation

Do not create one giant “MPR interface”.

Prefer focused abstractions:
- scene state
- pane view
- controller
- synchronization service
- tool adapter

### Dependency Inversion

The UI layer should depend on:
- controllers
- scene interfaces
- pane abstractions

not directly on VTK viewer details.

VTK-specific behavior should remain behind adapters.

## Phase Plan

### Phase 1

Build only:
- axial
- coronal
- sagittal
- shared world-space cursor state
- one active tool
- axis-aligned only

No 3D pane.
No thick slab.
No oblique.
No measurements.

### Phase 2

Add:
- proper crosshair overlays
- stable `WL/WW`
- stable `Pan`
- stable `Zoom`
- stable `Slice`

### Phase 3

Add:
- 4th pane: 3D reference view
- linked 2D / 3D cursor visualization

### Phase 4

Add:
- oblique mode
- thick slab
- blend modes

### Phase 5

Add:
- `Distance`
- `Angle`
- annotations if needed

## Anti-Patterns To Avoid

Do not repeat these mistakes:

- do not use pane-local slice numbers as the primary synchronization model
- do not let views own synchronization policy
- do not let multiple tool paths run at the same time
- do not let default VTK interaction remain enabled as hidden behavior
- do not add 3D reference view before the 3 orthogonal panes are stable
- do not keep layering debug shortcuts into the long-term structure

## Current Recommendation

When VTK MPR work resumes:

1. start from this document
2. implement only Phase 1
3. keep shared world-space cursor position as the core truth
4. derive pane state from that truth
5. treat the deleted VTK MPR code as discarded exploration

That is the correct restart point.*** End Patch
