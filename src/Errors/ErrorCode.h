#pragma once

enum class ErrorCode : int
{
    Unknown = 1000,
    InvalidOperation = 1001,
    MissingDependency = 1002,

    SeriesLoadMissingImageReference = 3000,
    SeriesLoadNullImageReference = 3001,
    SeriesLoadMissingFilePath = 3002,
    SeriesLoadReloadFailed = 3003,

    VolumeBuildInvalidInput = 3100,
    VolumeBuildInconsistentGeometry = 3101,
    VolumeBuildInconsistentOrientation = 3102,
    VolumeBuildInconsistentPixelSpacing = 3103,
    VolumeBuildInconsistentSliceSpacing = 3104,
    VolumeBuildDuplicateSlicePosition = 3105,

    VolumeRenderPreparationFailed = 3200,
    VolumeRenderMissingPreparedVolume = 3201,

    ThreeDPipelineIncompleteStrategySet = 3300,
    ThreeDPipelineNullSegmentationMask = 3301,
    ThreeDPipelineNullFilteredMask = 3302,
    ThreeDPipelineNullExtractedMesh = 3303,
    ThreeDPipelineNullPostProcessedMesh = 3304,

    DatabaseInitializationFailed = 3400,
    DatabaseImportFailed = 3401,

    ViewerOpenFailed = 3500,
    MprOpenFailed = 3501,
    ThreeDOpenFailed = 3502
};
