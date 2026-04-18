# 3D Pipeline Design Notes

## Purpose

This document explains the planned 3D pipeline for `DicomViewer` at three levels:

1. how data moves through the software
2. which algorithms are used at each step
3. the mathematics behind those algorithms

The goal is not only implementation guidance, but also to make later debugging and extension easier.

This document is written for a master's-level reader who needs enough mathematical and software context to reason about correctness.

## Scope

The planned first 3D pipeline is:

1. `VolumeBuilder`
2. `Isotropic Resampling`
3. `Gaussian Smoothing`
4. `Segmentation`
5. `Binary Mask Refinement`
6. `Connected Component Filtering`
7. `Marching Cubes`
8. `Normal Generation`
9. `Optional Mesh Smoothing`

For this project, the first practical segmentation strategy is:

- `Intensity Thresholding`
- optionally later `Region Growing`

This is enough for:

- bone extraction
- lung extraction
- high-contrast tissue boundaries

It is **not** a universal organ segmentation pipeline. More advanced organ isolation would later require stronger segmentation methods.

## Current Codebase Mapping

The existing project already contains part of the required pipeline.

### Existing relevant files

- [VolumeBuilder.cpp](/Users/goku/Documents/DicomViewer/src/Services/VolumeBuilder.cpp)
- [VolumeResampleService.cpp](/Users/goku/Documents/DicomViewer/src/Services/VolumeResampleService.cpp)
- [MprRenderService.cpp](/Users/goku/Documents/DicomViewer/src/Services/MprRenderService.cpp)
- [VolumeData.h](/Users/goku/Documents/DicomViewer/src/Model/VolumeData.h)
- [VolumeGeometry.h](/Users/goku/Documents/DicomViewer/src/Model/VolumeGeometry.h)
- [DicomImage.h](/Users/goku/Documents/DicomViewer/src/Model/DicomImage.h)
- [GDCMFileHandling.cpp](/Users/goku/Documents/DicomViewer/src/FileHandling/GDCMFileHandling.cpp)

### Already implemented

- DICOM slice metadata extraction
- slice ordering by geometry
- scalar volume construction
- isotropic resampling for MPR display

### Not yet implemented

- Gaussian smoothing service for 3D
- segmentation service
- connected component filtering service
- mesh data model
- marching cubes service
- mesh smoothing service

### Current implementation notes

The current 3D backend now includes:

- `MeshData<TScalar>`
- `SegmentationMaskData<TMaskVoxel>`
- `ThresholdSegmentationStrategy`
- `KeepLargestComponentStrategy`
- `MarchingCubesMeshExtractionStrategy`
- `MeshBuilder<float>`
- `MeshNormalGenerationPostProcessor`
- `LaplacianMeshSmoothingPostProcessor`

Current extraction behavior:

- extraction is still single-threaded
- a dedicated `MeshBuilder` now owns indexed vertex/triangle accumulation
- the builder deduplicates vertices by stable edge keys instead of emitting 3 fresh vertices per triangle

Optimization note:

- multithreading is intentionally deferred until the functional pipeline is complete and timed
- when we optimize later, extraction should run with one local `MeshBuilder` per slab/block and merge finished block meshes afterward
- likely later parallel stages:
  - isotropic resampling
  - Gaussian smoothing
  - threshold segmentation
  - mesh extraction

## High-Level Data Flow

The planned 3D workflow is:

```text
DICOM slices
  -> VolumeBuilder
  -> scalar volume V(x, y, z)
  -> Isotropic Resampling
  -> resampled scalar volume Vr
  -> Gaussian Smoothing
  -> smoothed volume Vs
  -> Segmentation
  -> binary mask M(x, y, z)
  -> Connected Component Filtering / refinement
  -> cleaned mask Mc
  -> Marching Cubes
  -> triangle mesh
  -> normals
  -> optional mesh smoothing
  -> 3D rendering / export
```

## Step 1: Volume Reconstruction

### Goal

Build a scalar 3D volume from an ordered DICOM series.

### Data

Each DICOM slice contributes:

- image width and height
- pixel values
- `ImagePositionPatient`
- `ImageOrientationPatient`
- `PixelSpacing`
- `SliceThickness`
- `SpacingBetweenSlices`

### Algorithm

The current code uses:

- orientation basis extraction
- slice normal computation
- projection of slice positions onto that normal
- geometric sorting
- voxel packing into a 3D scalar array

### Mathematical model

Let:

- `r` = row direction vector
- `c` = column direction vector
- `n = r x c` = slice normal

For a slice with patient-space origin `p_i`, define its slice coordinate as:

```text
s_i = p_i · n
```

where `·` is the dot product.

Sorting slices by `s_i` produces the physical slice order.

### Output

A scalar volume:

```text
V(i, j, k)
```

with geometry:

- dimensions `(Nx, Ny, Nz)`
- spacing `(sx, sy, sz)`
- origin `o`
- direction matrix `R = [r c n]`

### Debugging questions

If volume assembly looks wrong, check:

- are slices sorted in physical order?
- is orientation consistent across slices?
- is `sz` correct?
- does `origin` match the first slice center?

## Step 2: Isotropic Resampling

### Why it is needed

Many CT/MR series are anisotropic:

- in-plane spacing may be small
- slice spacing may be much larger

If marching cubes runs directly on anisotropic voxels, the reconstructed 3D surface looks distorted or stair-stepped.

### Goal

Resample the volume to isotropic spacing:

```text
sx = sy = sz = h
```

where `h` is typically the smallest source spacing.

### Mathematical model

We want a new volume:

```text
Vr(u, v, w)
```

defined on a new isotropic grid.

Each target voxel center in world coordinates is mapped back into source volume coordinates.

If:

- `x_world` is a target voxel center in patient coordinates
- `R` is the source direction matrix
- `o` is the source origin
- `S = diag(sx, sy, sz)`

then source index coordinates are:

```text
x_index = S^-1 R^T (x_world - o)
```

The source value is then interpolated from neighboring voxels.

### Interpolation

Use `trilinear interpolation`.

If the continuous source coordinate is:

```text
(x, y, z)
```

with:

- `i = floor(x)`
- `j = floor(y)`
- `k = floor(z)`
- `dx = x - i`
- `dy = y - j`
- `dz = z - k`

then the interpolated value is the weighted sum of the 8 voxel corners:

```text
Vr(x, y, z) =
(1-dx)(1-dy)(1-dz) V(i,   j,   k  ) +
dx    (1-dy)(1-dz) V(i+1, j,   k  ) +
(1-dx)dy    (1-dz) V(i,   j+1, k  ) +
dx    dy    (1-dz) V(i+1, j+1, k  ) +
(1-dx)(1-dy)dz     V(i,   j,   k+1) +
dx    (1-dy)dz     V(i+1, j,   k+1) +
(1-dx)dy    dz     V(i,   j+1, k+1) +
dx    dy    dz     V(i+1, j+1, k+1)
```

### Debugging questions

- do coronal and sagittal views preserve anatomy?
- does the resampled volume cover the same world-space extent?
- are target dimensions consistent with spacing and extent?

## Step 3: Gaussian Smoothing

### Goal

Reduce noise before segmentation and surface extraction.

### Why it helps

Marching cubes is sensitive to local noise.
Without smoothing:

- surfaces can become jagged
- tiny high-frequency structures become false geometry

### Mathematical model

Apply a 3D Gaussian convolution:

```text
Vs = G_sigma * Vr
```

where the Gaussian kernel is:

```text
G_sigma(x, y, z) = (1 / ((2pi)^(3/2) sigma^3)) exp(-(x^2 + y^2 + z^2) / (2 sigma^2))
```

In practice, use a discrete separable kernel:

```text
G3D(x, y, z) = G1D(x) G1D(y) G1D(z)
```

This is computationally cheaper than full 3D convolution.

### Parameter

- `sigma`

Larger `sigma`:

- stronger smoothing
- less noise
- more anatomical boundary loss

Smaller `sigma`:

- more detail
- less stable segmentation

### Debugging questions

- is the volume too blurred?
- are thin structures disappearing?
- are threshold results becoming more stable?

## Step 4: Segmentation

### Goal

Create a binary mask for the target anatomy.

### Output

```text
M(x, y, z) in {0, 1}
```

where:

- `1` means inside target
- `0` means background

### First practical method: Intensity Thresholding

For CT, thresholding is often effective for:

- bone
- lung
- air spaces

Define:

```text
M(x, y, z) =
1, if T_low <= Vs(x, y, z) <= T_high
0, otherwise
```

Examples:

- bone: high HU threshold
- lung: very low HU threshold

### Limits

Thresholding alone is weak for:

- soft tissue organs with overlapping intensities
- noisy data
- contrast-varying studies

### Later extension: Region Growing

Given a seed voxel `p0`, grow a region by adding neighboring voxels that satisfy:

```text
|Vs(p) - mu_region| <= epsilon
```

or:

```text
Vs(p) in acceptable range
```

This helps isolate specific organs once a seed is provided.

### Debugging questions

- is the threshold too wide or too narrow?
- are target voxels missing?
- is background leaking into the mask?

## Step 5: Binary Mask Refinement

### Goal

Clean the binary mask before 3D extraction.

### Typical operations

- hole filling
- morphological opening
- morphological closing
- removal of isolated speckles

### Morphological intuition

With a structuring element `B`:

- erosion shrinks the object
- dilation expands the object

Opening:

```text
M_open = (M eroded by B) dilated by B
```

removes small noise.

Closing:

```text
M_close = (M dilated by B) eroded by B
```

fills small holes and gaps.

### Debugging questions

- are holes in the structure preserved or incorrectly filled?
- are thin bridges broken?
- is noise still present after cleanup?

## Step 6: Connected Component Filtering

### Goal

Keep only meaningful connected regions.

### Mathematical model

Treat the mask as a graph:

- each foreground voxel is a node
- edges connect neighboring foreground voxels

Neighborhood choice:

- 6-connected
- 18-connected
- 26-connected

Each connected component `C_i` has size:

```text
|C_i|
```

Then keep:

- the largest component
- or the top `k` components
- or only components above a volume threshold

### Why it matters

After thresholding, small false positives often remain.
Connected component filtering removes them.

### Debugging questions

- are both lungs kept or only one?
- is the largest component really the target anatomy?
- are small true structures being removed?

## Step 7: Marching Cubes

### Goal

Extract a triangle mesh from the binary mask or scalar field.

### Core idea

Marching cubes processes each voxel cell independently.

Each cube has:

- 8 corner samples

Each corner is either:

- below the isovalue
- above the isovalue

That gives:

```text
2^8 = 256
```

possible cube configurations.

These reduce to a smaller number of unique topological cases by symmetry.

### Mathematical model

Given scalar field:

```text
f(x, y, z)
```

and isovalue:

```text
f(x, y, z) = tau
```

Marching cubes approximates this implicit surface by triangles.

For each cube edge whose endpoints have opposite signs relative to `tau`, find the intersection by linear interpolation.

If edge endpoints are:

- `p1`, `p2`
- with scalar values `f1`, `f2`

then the intersection point is:

```text
p = p1 + ((tau - f1) / (f2 - f1)) (p2 - p1)
```

### In binary-mask mode

If using the cleaned mask directly:

- foreground = `1`
- background = `0`
- isovalue is typically `0.5`

### Output

A triangle mesh:

```text
vertices + faces
```

### Debugging questions

- are surfaces topologically broken?
- is the mesh too jagged?
- are there floating fragments?

## Step 8: Normal Generation

### Goal

Make the mesh shade correctly under lighting.

### Mathematical model

For triangle with vertices:

- `v0`
- `v1`
- `v2`

face normal:

```text
n_face = (v1 - v0) x (v2 - v0)
```

then normalize:

```text
n_face = n_face / ||n_face||
```

Vertex normals are usually computed by averaging adjacent face normals:

```text
n_vertex = normalize(sum_i w_i n_face_i)
```

with optional area-based weights `w_i`.

### Debugging questions

- is lighting inverted?
- are normals consistent across the surface?
- are hard edges expected or unwanted?

## Step 9: Optional Mesh Smoothing

### Goal

Reduce staircase artifacts after surface extraction.

### Practical first method: Laplacian smoothing

For vertex `v_i` with neighbor set `N(i)`:

```text
v_i_new = v_i + lambda ( (1 / |N(i)|) sum_{j in N(i)} v_j - v_i )
```

where:

- `lambda` controls smoothing strength

### Risk

Laplacian smoothing causes shrinkage.
This is why it should be:

- optional
- mild

### Debugging questions

- is the mesh shrinking too much?
- are anatomical edges being rounded away?

## Recommended First Implementable Pipeline

For this project, the best first 3D pipeline is:

```text
VolumeBuilder
-> Isotropic Resampling
-> Gaussian Smoothing
-> Intensity Thresholding
-> Connected Component Filtering
-> Marching Cubes
-> Normal Generation
-> Optional Mesh Smoothing
```

This should work well first for:

- CT bone
- lung
- other high-contrast structures

## Recommended Class-Level Design

Suggested future services:

- `SegmentationMaskData`
- `SegmentationService`
- `ConnectedComponentService`
- `MeshData`
- `MarchingCubesService`
- `MeshSmoothingService`

Suggested responsibilities:

- `VolumeBuilder`
  - build scalar volume from DICOM series
- `VolumeResampleService`
  - create isotropic volume
- `SegmentationService`
  - threshold / region growing
- `ConnectedComponentService`
  - keep relevant mask components
- `MarchingCubesService`
  - generate mesh
- `MeshSmoothingService`
  - optional mesh refinement

This keeps the code close to SOLID:

- each service has a single responsibility
- segmentation and geometry extraction remain separate
- debugging stays local to each stage

## How To Debug The Full Pipeline

The most important rule:

- inspect output after every stage

Do **not** debug only at the final mesh.

Recommended debug artifacts:

1. volume slices after reconstruction
2. resampled coronal/sagittal slices
3. smoothed slices
4. binary threshold mask slices
5. cleaned connected-component mask slices
6. raw marching cubes mesh
7. final shaded mesh

If the final mesh is wrong:

- first check the mask
- if the mask is wrong, check thresholding
- if thresholding is unstable, check smoothing and resampling
- if anatomy shape is wrong before segmentation, check volume geometry

## Common Failure Modes

### Wrong 3D shape

Usually caused by:

- incorrect spacing
- incorrect slice order
- bad resampling extent

### Empty mesh

Usually caused by:

- threshold too strict
- segmentation mask empty
- wrong isovalue

### Too many floating surfaces

Usually caused by:

- noisy threshold mask
- missing connected component filtering

### Jagged surface

Usually caused by:

- anisotropic input
- no smoothing
- too coarse spacing

## Final Practical Recommendation

Implement in this order:

1. `MeshData`
2. `SegmentationService` with thresholding
3. `ConnectedComponentService`
4. `MarchingCubesService`
5. `Normal Generation`
6. optional smoothing

Do not start with a fully general organ segmentation system.
Start with a high-contrast, threshold-friendly target.

The best first target is:

- CT bone

Then move to:

- lung

Then later:

- organ-specific segmentation

That progression is technically safer and much easier to validate.

## Test Strategy

This section describes how to test the full 3D pipeline like a production-quality software project.

The key rule is:

- test each stage in isolation
- then test the pipeline end to end
- then keep a small regression dataset so later refactors cannot silently break geometry or mesh output

The pipeline should be tested at three levels:

1. unit tests
2. integration tests
3. regression / golden-data tests

## Testing Principles

### 1. Prefer synthetic data when possible

Synthetic volumes make it easier to verify:

- exact geometry
- exact spacing
- expected threshold masks
- expected mesh topology

Examples:

- cube phantom
- sphere phantom
- cylinder phantom
- two disconnected spheres

These are much easier to validate than real CT first.

### 2. Use real DICOM series only after synthetic coverage exists

Real datasets are needed for:

- geometry consistency checks
- clinical realism
- visual QA

But they are weaker for strict mathematical assertions because:

- spacing is imperfect
- intensities are noisy
- anatomy varies

### 3. Assert invariants, not only screenshots

Visual output is important, but tests should assert measurable properties:

- dimensions
- spacing
- voxel ranges
- mask voxel counts
- connected component counts
- triangle counts
- bounding boxes
- surface area / volume tolerance

## Recommended Test Layout

Suggested future structure:

```text
tests/
  unit/
    VolumeBuilderTests.cpp
    VolumeResampleServiceTests.cpp
    SegmentationServiceTests.cpp
    ConnectedComponentServiceTests.cpp
    MarchingCubesServiceTests.cpp
    MeshSmoothingServiceTests.cpp
  integration/
    PipelineIntegrationTests.cpp
    RealDicomSeriesTests.cpp
  data/
    synthetic/
    dicom/
```

If the project later adopts a test framework such as `Catch2` or `GoogleTest`, keep the same conceptual structure.

## Stage-by-Stage Test Cases

### A. VolumeBuilder Tests

#### A1. Slices are ordered correctly by geometry

Input:

- 3 synthetic slices
- same width/height
- same orientation
- out-of-order `ImagePositionPatient`

Expected:

- output volume depth is `3`
- slices are stored in physical order
- first slice in volume corresponds to lowest projected normal coordinate

Assertions:

- `dimensions.z == 3`
- `origin == firstSortedSlicePosition`
- voxel values at `z=0,1,2` match expected slice IDs

#### A2. Orientation basis is orthonormalized

Input:

- synthetic orientation vectors that are nearly, but not exactly, orthogonal

Expected:

- output row, column, normal are unit length
- dot products are approximately zero

Assertions:

```text
|r| approx 1
|c| approx 1
|n| approx 1
r · c approx 0
r · n approx 0
c · n approx 0
```

#### A3. Inconsistent series is rejected

Input:

- one slice with different dimensions
- or inconsistent pixel spacing
- or inconsistent orientation

Expected:

- build fails

Assertions:

- throws expected exception or returns failure result

#### A4. Duplicate slice position is rejected

Input:

- two slices with the same projected coordinate

Expected:

- build fails during geometry validation

### How to implement A-tests

Do not use real DICOM first.
Construct `DicomImage` objects directly in memory with:

- width / height
- raw pixels
- `ImagePositionPatient`
- `ImageOrientationPatient`
- `PixelSpacing`

This keeps the tests deterministic and fast.

## B. Isotropic Resampling Tests

#### B1. Identity resampling preserves volume

Input:

- isotropic source volume
- target spacing equal to source spacing

Expected:

- same dimensions
- same spacing
- voxel values approximately identical

Assertions:

- output geometry equals input geometry
- selected voxel samples match exactly or within tolerance

#### B2. Anisotropic to isotropic resampling gives correct dimensions

Input:

- dimensions `(10, 10, 5)`
- spacing `(1.0, 1.0, 2.0)`

Expected:

- target spacing `1.0`
- target z dimension:

```text
round((5 - 1) * 2.0 / 1.0) + 1 = 9
```

Assertions:

- output spacing `(1,1,1)`
- output dimensions `(10,10,9)`

#### B3. World-space extent is preserved

Input:

- known origin, direction, spacing, dimensions

Expected:

- center-to-center extent preserved after resampling

Assertions:

- output `worldMin == input worldMin`
- output `worldMax approx input worldMax`

#### B4. Trilinear interpolation behaves correctly

Input:

- tiny `2x2x2` source volume with known values

Expected:

- interpolated value at center equals weighted average of all 8 corners

Assertions:

- numeric result matches hand-computed value

### How to implement B-tests

Create tiny synthetic scalar volumes in code.
These tests should not depend on DICOM loading at all.

## C. Gaussian Smoothing Tests

#### C1. Constant volume remains constant

Input:

- all voxels equal `K`

Expected:

- smoothed result remains `K`

Assertions:

- every voxel equals `K` within tolerance

#### C2. Impulse response becomes Gaussian-like

Input:

- one voxel set to `1`
- all others `0`

Expected:

- output is symmetric
- center value is maximum
- neighbors decay with distance

Assertions:

- symmetry across axes
- monotonic local decrease away from center

#### C3. Noise energy decreases

Input:

- noisy synthetic volume

Expected:

- variance decreases after smoothing

Assertions:

- `var(output) < var(input)`

## D. Segmentation Tests

#### D1. Threshold segmentation on synthetic cube

Input:

- background intensity `0`
- centered cube intensity `100`
- threshold range `[50, 150]`

Expected:

- mask contains only the cube

Assertions:

- foreground voxel count equals expected cube volume
- outside region is zero

#### D2. Threshold excludes out-of-range intensities

Input:

- volume with three classes: `0`, `50`, `200`
- threshold `[100, 300]`

Expected:

- only `200` region remains

#### D3. Region growing later

Input:

- seeded sphere phantom

Expected:

- only connected similar-intensity region is grown

Assertions:

- region count and size within expected tolerance

## E. Connected Component Tests

#### E1. Largest component only

Input:

- binary mask with components of size `100`, `20`, `5`

Expected:

- only size `100` remains

Assertions:

- one component remains
- voxel count equals `100`

#### E2. Two lungs case

Input:

- two large components and several small noise blobs

Expected:

- top two components preserved if configured

Assertions:

- exactly two large components remain

#### E3. Connectivity sensitivity

Input:

- diagonally touching voxels

Expected:

- different result for 6-connected vs 26-connected mode

Assertions:

- component count differs as expected

## F. Marching Cubes Tests

#### F1. Empty mask produces empty mesh

Input:

- all-zero mask

Expected:

- no vertices
- no triangles

#### F2. Full mask produces boundary surface

Input:

- all-one volume

Expected:

- outer boundary mesh only

Assertions:

- triangle count greater than zero
- mesh bounding box matches volume extent

#### F3. Cube phantom produces cube-like mesh

Input:

- binary cube phantom

Expected:

- mesh bounding box matches cube bounds within tolerance

Assertions:

- mesh min/max coordinates near expected cube corners
- no disconnected fragments

#### F4. Sphere phantom approximates sphere

Input:

- binary sphere mask

Expected:

- mesh surface area and volume approximate analytical sphere values

Analytical values:

```text
Volume = 4/3 pi r^3
Area   = 4 pi r^2
```

Assertions:

- computed mesh bounding box radius approximately `r`
- optional approximate surface area / enclosed volume within tolerance

### How to implement F-tests

Start with binary masks, not scalar CT.
This isolates mesh extraction from segmentation uncertainty.

## G. Normal Generation Tests

#### G1. Face normals are unit length

Assertions:

- `||n|| approx 1`

#### G2. Normals point outward on convex phantom

Input:

- sphere or cube phantom

Expected:

- vertex normals align roughly with outward radial direction

Assertions:

- `n · (v - center) > 0`

## H. Mesh Smoothing Tests

#### H1. Vertex count remains unchanged

Expected:

- smoothing changes positions only

Assertions:

- same number of vertices and faces before/after

#### H2. Roughness decreases

Possible metric:

- mean local normal variation
- or average Laplacian magnitude

Expected:

- roughness metric decreases after smoothing

#### H3. Shrinkage stays bounded

Expected:

- bounding box or enclosed volume decreases only within configured tolerance

## Integration Tests

Integration tests should connect multiple stages together.

### I1. Synthetic end-to-end bone-like pipeline

Input:

- synthetic high-intensity object in low-intensity background

Pipeline:

- resample
- smooth
- threshold
- connected components
- marching cubes

Expected:

- final mesh exists
- no extra fragments
- bounding box matches source object

### I2. Real DICOM CT series end-to-end

Input:

- one small known CT dataset

Pipeline:

- `VolumeBuilder`
- `VolumeResampleService`
- threshold segmentation
- marching cubes

Assertions:

- no crash
- no empty intermediate outputs
- final mesh triangle count within expected range

### I3. MPR / 3D consistency

Input:

- known volume

Expected:

- segmentation mask seen in slices matches extracted 3D extent

This is important because sometimes 3D appears wrong when the real problem is in preprocessing.

## Regression Tests

Regression tests prevent future refactors from silently breaking behavior.

### Suggested regression datasets

Keep a very small set of known-good cases:

1. synthetic cube phantom
2. synthetic sphere phantom
3. one CT head dataset
4. one CT chest dataset

Store expected measurements:

- volume dimensions
- resampled dimensions
- mask voxel count
- connected component count
- mesh vertex count
- mesh triangle count
- mesh bounding box

These become golden references.

### Regression tolerance

Not every floating-point stage should require exact equality.
Use tolerances.

Examples:

- spacing: exact or very small epsilon
- interpolation results: small epsilon
- mesh counts: exact if algorithm deterministic
- surface area / volume: percentage tolerance

## Suggested Assertions Per Stage

Minimum assertions to log or test:

### Volume

- dimensions
- spacing
- origin
- direction matrix orthogonality
- voxel min/max

### Segmentation

- threshold used
- foreground voxel count
- connected component count
- largest component size

### Mesh

- vertex count
- triangle count
- bounding box
- normal validity

## How To Implement Tests In This Project

### Step 1. Add a test framework

Use one of:

- `Catch2`
- `GoogleTest`

For this project, `Catch2` would be a simple fit for CMake and C++20.

### Step 2. Keep test utilities small

Add helper builders for:

- synthetic scalar volumes
- synthetic binary masks
- fake `DicomImage` slice series
- mesh measurement helpers

Suggested helper files:

```text
tests/helpers/SyntheticVolumeFactory.h
tests/helpers/SyntheticDicomFactory.h
tests/helpers/MeshTestUtils.h
```

### Step 3. Test services, not UI

Do not start by testing Qt windows.
The 3D pipeline should be tested mostly below the UI layer:

- service classes
- model classes
- math utilities

UI should have only lightweight smoke tests later.

### Step 4. Save debug artifacts when a test fails

For difficult failures, it is useful to optionally dump:

- a few slices as PNG
- a binary mask as PNG stack
- a mesh as OBJ

That makes debugging faster than only reading assertion failures.

## Example Test Matrix

### Unit

- `VolumeBuilder_SortsSlicesByGeometry`
- `VolumeBuilder_RejectsMismatchedSpacing`
- `Resample_PreservesCenterExtent`
- `GaussianSmoothing_PreservesConstantField`
- `Threshold_CreatesExpectedCubeMask`
- `ConnectedComponents_KeepLargestWorks`
- `MarchingCubes_EmptyMaskProducesNoTriangles`
- `MarchingCubes_SphereApproximationReasonable`

### Integration

- `Pipeline_CubePhantom_GeneratesExpectedMesh`
- `Pipeline_HeadCT_BoneThreshold_GeneratesNonEmptyMesh`
- `Pipeline_ChestCT_LungThreshold_GeneratesTwoMainComponents`

### Regression

- `Regression_HeadCT_MeshTriangleCountStable`
- `Regression_ChestCT_ResampledDimensionsStable`

## What To Test First

Best first test order:

1. `VolumeBuilder`
2. `VolumeResampleService`
3. threshold segmentation
4. connected components
5. marching cubes
6. one synthetic end-to-end integration test

Do not start with a real CT regression suite before synthetic tests exist.

## Engineering Recommendation

If this is implemented like a real software system, each stage should have:

- deterministic input
- deterministic output
- measurable invariants
- isolated tests

The strongest design is:

- geometry tested independently
- preprocessing tested independently
- segmentation tested independently
- mesh generation tested independently
- only then pipeline integration tested

That gives you a system that is much easier to trust, extend, and debug later.
