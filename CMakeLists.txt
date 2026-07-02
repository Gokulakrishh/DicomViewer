cmake_minimum_required(VERSION 3.21)

set(DICOMVIEWER_PRODUCT_NAME "CrossAxialDicomViewer" CACHE STRING "Internal product name")
set(DICOMVIEWER_DISPLAY_NAME "Cross Axial Dicom Viewer" CACHE STRING "User-facing product name")
set(DICOMVIEWER_BUNDLE_IDENTIFIER "com.crossaxialdicomviewer.app" CACHE STRING "macOS bundle identifier")
set(DICOMVIEWER_VERSION_MAJOR "1" CACHE STRING "Major version")
set(DICOMVIEWER_VERSION_MINOR "0" CACHE STRING "Minor version")
set(DICOMVIEWER_VERSION_RELEASE "0" CACHE STRING "Release/Patch version")
set(DICOMVIEWER_VERSION_BUILD "1" CACHE STRING "Build version")

set(DICOMVIEWER_VERSION_SHORT
    "${DICOMVIEWER_VERSION_MAJOR}.${DICOMVIEWER_VERSION_MINOR}.${DICOMVIEWER_VERSION_RELEASE}")
set(DICOMVIEWER_VERSION_STRING
    "${DICOMVIEWER_VERSION_MAJOR}.${DICOMVIEWER_VERSION_MINOR}.${DICOMVIEWER_VERSION_RELEASE}.${DICOMVIEWER_VERSION_BUILD}")

project(${DICOMVIEWER_PRODUCT_NAME}
    VERSION ${DICOMVIEWER_VERSION_SHORT}
    DESCRIPTION "Cross Axial DICOM Viewer (2D/3D)"
    LANGUAGES C CXX
)

include(CTest)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Product: ${DICOMVIEWER_DISPLAY_NAME}")
message(STATUS "Version: ${DICOMVIEWER_VERSION_STRING}")
add_compile_definitions(__DicomViewer_BUILD_TYPE="${CMAKE_BUILD_TYPE}")

# ----------------------------
# Global C++ settings
# ----------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(DICOMVIEWER_ENABLE_VTK "Enable VTK-based 3D volume rendering integration" ON)
option(DICOMVIEWER_ENABLE_MEMORY_LOGGING "Enable memory management debug logging" OFF)

set(DICOMVIEWER_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${DICOMVIEWER_GENERATED_DIR}")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Utilities/AppVersion.h.in"
    "${DICOMVIEWER_GENERATED_DIR}/AppVersion.h"
    @ONLY
)

if(DICOMVIEWER_ENABLE_MEMORY_LOGGING)
    add_compile_definitions(DICOMVIEWER_ENABLE_MEMORY_LOGGING=1)
else()
    add_compile_definitions(DICOMVIEWER_ENABLE_MEMORY_LOGGING=0)
endif()

# ----------------------------
# Find dependencies
# ----------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql Concurrent Network)
find_package(Qt6 QUIET COMPONENTS Qml Quick QuickWidgets Quick3D)
message(STATUS "Qt major version: 6")

set(DICOMVIEWER_ENABLE_QML3D OFF)
if(TARGET Qt6::Qml AND TARGET Qt6::Quick AND TARGET Qt6::QuickWidgets AND TARGET Qt6::Quick3D)
    set(DICOMVIEWER_ENABLE_QML3D ON)
    message(STATUS "QML 3D viewer enabled")
else()
    message(STATUS "QML 3D viewer disabled: Qt Qml/Quick/QuickWidgets/Quick3D stack not fully available")
endif()

find_package(GDCM REQUIRED)
message(STATUS "GDCM")

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER_CORE REQUIRED IMPORTED_TARGET gstreamer-1.0>=1.24)
pkg_check_modules(GSTREAMER_APP REQUIRED IMPORTED_TARGET gstreamer-app-1.0>=1.24)
if(NOT GSTREAMER_CORE_VERSION MATCHES "^1\\.24\\." OR
   NOT GSTREAMER_APP_VERSION MATCHES "^1\\.24\\.")
    message(FATAL_ERROR
        "Cross Axial Dicom Viewer requires GStreamer 1.24.x; found core "
        "${GSTREAMER_CORE_VERSION}, app ${GSTREAMER_APP_VERSION}")
endif()
message(STATUS "GStreamer version: ${GSTREAMER_CORE_VERSION}")

if(APPLE)
    # MacPorts pkg-config adds a generic /opt/local/lib rpath. That directory
    # may also contain a second VTK build, which can satisfy the selected VTK
    # installation's @rpath references and load two incompatible VTK copies.
    # GStreamer/GDCM dylibs use absolute install names, so this rpath is neither
    # required nor safe for the application target.
    foreach(gstreamer_target IN ITEMS PkgConfig::GSTREAMER_CORE PkgConfig::GSTREAMER_APP)
        get_target_property(gstreamer_link_options ${gstreamer_target} INTERFACE_LINK_OPTIONS)
        if(gstreamer_link_options)
            list(FILTER gstreamer_link_options EXCLUDE REGEX "^-Wl,-rpath,/opt/local/lib$")
            set_property(
                TARGET ${gstreamer_target}
                PROPERTY INTERFACE_LINK_OPTIONS "${gstreamer_link_options}"
            )
        endif()
    endforeach()
endif()

set(APP_3D_MODEL_SOURCES
    src/Model/MeshData.cpp
    src/Model/MeshBuilder.cpp
    src/Model/SegmentationMaskData.cpp
)

set(APP_3D_MODEL_HEADERS
    src/Model/MeshConcepts.h
    src/Model/IMeshData.h
    src/Model/MeshData.h
    src/Model/MeshData.tpp
    src/Model/MeshBuilder.h
    src/Model/MeshBuilder.tpp
    src/Model/ISegmentationMask.h
    src/Model/SegmentationMaskData.h
    src/Model/SegmentationMaskData.tpp
)

set(APP_3D_SERVICE_SOURCES
    src/Services/CompositeMeshPostProcessor.cpp
    src/Services/SeriesDataLoadService.cpp
    src/Services/ThreeDSeriesBuildService.cpp
    src/Services/ThreeDimensionalPipelineService.cpp
    src/Services/VolumeRenderPreprocessingService.cpp
    src/Services/ThreeDProfiles/Bone3dPipelineProfile.cpp
    src/Services/ThreeDProfiles/Lung3dPipelineProfile.cpp
    src/Services/ThreeDProfiles/ThreeDProfileSelection.cpp
    src/Services/ThreeDMesh/LaplacianMeshSmoothingPostProcessor.cpp
    src/Services/ThreeDMesh/MarchingCubesMeshExtractionStrategy.cpp
    src/Services/ThreeDMesh/MeshNormalGenerationPostProcessor.cpp
    src/Services/ThreeDSegmentation/KeepLargestNComponentsStrategy.cpp
    src/Services/ThreeDSegmentation/ThresholdSegmentationStrategy.cpp
)

set(APP_3D_SERVICE_HEADERS
    src/Services/CompositeMeshPostProcessor.h
    src/Services/IConnectedComponentStrategy.h
    src/Services/IMeshExtractionStrategy.h
    src/Services/IMeshPostProcessor.h
    src/Services/ISegmentationStrategy.h
    src/Services/SeriesDataLoadService.h
    src/Services/ThreeDSeriesBuildService.h
    src/Services/ThreeDimensionalPipelineResult.h
    src/Services/ThreeDimensionalPipelineService.h
    src/Services/VolumeRenderPreprocessingService.h
    src/Services/ThreeDProfiles/Bone3dPipelineProfile.h
    src/Services/ThreeDProfiles/I3dPipelineProfile.h
    src/Services/ThreeDProfiles/Lung3dPipelineProfile.h
    src/Services/ThreeDProfiles/ThreeDProfileSelection.h
    src/Services/ThreeDMesh/LaplacianMeshSmoothingPostProcessor.h
    src/Services/ThreeDMesh/MarchingCubesMeshExtractionStrategy.h
    src/Services/ThreeDMesh/MeshNormalGenerationPostProcessor.h
    src/Services/ThreeDSegmentation/KeepLargestNComponentsStrategy.h
    src/Services/ThreeDSegmentation/ThresholdSegmentationStrategy.h
)

set(APP_QML3D_SOURCES)
set(APP_QML3D_HEADERS)
set(APP_QML3D_VIEWER_SOURCES)
set(APP_QML3D_VIEWER_HEADERS)

if(DICOMVIEWER_ENABLE_QML3D)
    list(APPEND APP_QML3D_SOURCES
        src/Qml3D/MeshSceneAdapter.cpp
        src/Qml3D/QmlMeshGeometry.cpp
        src/Qml3D/ThreeDViewerController.cpp
    )

    list(APPEND APP_QML3D_HEADERS
        src/Qml3D/MeshSceneAdapter.h
        src/Qml3D/QmlMeshGeometry.h
        src/Qml3D/ThreeDViewerController.h
    )

    list(APPEND APP_QML3D_VIEWER_SOURCES
        src/AdvancedViewer/ThreeDViewerWindow.cpp
    )

    list(APPEND APP_QML3D_VIEWER_HEADERS
        src/AdvancedViewer/ThreeDViewerWindow.h
    )
endif()

set(APP_VTK_VIEWER_SOURCES)
set(APP_VTK_VIEWER_HEADERS)

if(DICOMVIEWER_ENABLE_VTK)
    find_package(VTK REQUIRED COMPONENTS
        CommonCore
        CommonDataModel
        FiltersModeling
        FiltersSources
        InteractionWidgets
        GUISupportQt
        IOImage
        InteractionImage
        InteractionStyle
        RenderingImage
        RenderingCore
        RenderingOpenGL2
        RenderingVolumeOpenGL2
    )
    message(STATUS "VTK version: ${VTK_VERSION}")

    list(APPEND APP_VTK_VIEWER_SOURCES
        src/VTK/Adapters/VtkVolumeAdapter.cpp
        src/VTK/MainView/VtkSliceSceneAdapter.cpp
        src/VTK/MainView/VtkDiagnosticSliceView.cpp
        src/VTK/MainView/DicomMeasurementScalarSource.cpp
        src/ViewerTools/CrosshairTool.cpp
        src/ViewerTools/Measurements/MeasurementAnalyticsService.cpp
        src/ViewerTools/Measurements/MeasurementService.cpp
        src/ViewerTools/Measurements/MeasurementTool.cpp
        src/ViewerTools/PanTool.cpp
        src/ViewerTools/ViewerToolPresentation.cpp
        src/ViewerTools/WindowLevelTool.cpp
        src/ViewerTools/ZoomTool.cpp
        src/VTK/MPR/Adapters/VtkMprSceneAdapter.cpp
        src/VTK/MPR/Adapters/MprToolAdapter.cpp
        src/VTK/MPR/MprMeasurementScalarSource.cpp
        src/VTK/MPR/Controllers/InteractionRouter.cpp
        src/VTK/MPR/Controllers/MprController.cpp
        src/VTK/MPR/Controllers/ToolController.cpp
        src/VTK/MPR/State/MprScene.cpp
        src/VTK/MPR/Sync/MprSynchronizationService.cpp
        src/VTK/MPR/View/VtkMprView.cpp
        src/VTK/MPR/View/VtkSliceMprPaneView.cpp
        src/VTK/MPR/View/VtkThreeDReferencePaneView.cpp
        src/VTK/MPR/Window/VtkMprViewerWindow.cpp
        src/VTK/Camera/VtkVolumeCameraController.cpp
        src/VTK/Presets/VtkVolumeRenderPreset.cpp
        src/VTK/Presets/VtkVolumePresetLibrary.cpp
        src/VTK/Viewer/VtkVolumeViewerWindow.cpp
    )

    list(APPEND APP_VTK_VIEWER_HEADERS
        src/VTK/Adapters/VtkVolumeAdapter.h
        src/VTK/MainView/VtkSliceSceneAdapter.h
        src/VTK/MainView/VtkDiagnosticSliceView.h
        src/ViewerTools/CrosshairTool.h
        src/ViewerTools/IViewerTool.h
        src/ViewerTools/IViewerToolTarget.h
        src/ViewerTools/Measurements/IMeasurementScalarSource.h
        src/ViewerTools/Measurements/MeasurementAnalyticsService.h
        src/ViewerTools/Measurements/MeasurementService.h
        src/ViewerTools/Measurements/MeasurementTool.h
        src/ViewerTools/Measurements/MeasurementTypes.h
        src/VTK/MainView/DicomMeasurementScalarSource.h
        src/ViewerTools/PanTool.h
        src/ViewerTools/ViewerInputEvent.h
        src/ViewerTools/ViewerToolPresentation.h
        src/ViewerTools/WindowLevelPreset.h
        src/ViewerTools/WindowLevelTool.h
        src/ViewerTools/ZoomTool.h
        src/ViewerOverlays/CrosshairOverlayWidget.h
        src/ViewerOverlays/MeasurementOverlayWidget.h
        src/VTK/MPR/Adapters/MprToolAdapter.h
        src/VTK/MPR/Adapters/VtkMprSceneAdapter.h
        src/VTK/MPR/Controllers/InteractionRouter.h
        src/VTK/MPR/Controllers/MprController.h
        src/VTK/MPR/Controllers/ToolController.h
        src/VTK/MPR/MprMeasurementScalarSource.h
        src/VTK/MPR/MprTypes.h
        src/VTK/MPR/State/MprScene.h
        src/VTK/MPR/Sync/MprSynchronizationService.h
        src/VTK/MPR/View/IMprPaneView.h
        src/VTK/MPR/View/VtkMprView.h
        src/VTK/MPR/View/VtkSliceMprPaneView.h
        src/VTK/MPR/View/VtkThreeDReferencePaneView.h
        src/VTK/MPR/Window/VtkMprViewerWindow.h
        src/VTK/Camera/VtkVolumeCameraController.h
        src/VTK/Presets/VtkVolumeRenderPreset.h
        src/VTK/Presets/VtkVolumePresetLibrary.h
        src/VTK/Viewer/VtkVolumeViewerWindow.h
    )
endif()
 
# ----------------------------
# Source files
# ----------------------------
set(SRC_FILES
    src/Main/main.cpp
    src/resources/styles.qrc
    src/AI/GeminiAiAssistantService.cpp
    src/AI/QtHttpAiServerClient.cpp
    src/Audit/AuditService.cpp
    src/Audit/DicomMetadataAudit.cpp
    src/Audit/ErrorAuditAdapter.cpp
    src/Audit/JsonlAuditSink.cpp
    src/AdvancedViewer/WidgetsAdvancedViewerLauncher.cpp
    src/Errors/QtErrorDialogPresenter.cpp
    src/Utilities/AppStyle.cpp
    src/Utilities/AppIcons.cpp
    src/Utilities/ApplicationPaths.cpp
    src/Utilities/AppDialogBase.cpp
    src/Utilities/AiApiKeyDialog.cpp
    src/Utilities/CrashReportService.cpp
    src/Utilities/MemoryManagementDebug.cpp
    src/Utilities/LoadingDialog.cpp
    src/Utilities/QSettingsAppConfigService.cpp
    src/Utilities/WarningDialog.cpp
    src/FileHandling/GDCMFileHandling.cpp
    src/DicomViewerWindow/DicomPreviewPanel.cpp
    src/DicomViewerWindow/AnnotationReportDock.cpp
    src/DicomViewerWindow/AnnotationSliceBrowserPanel.cpp
    src/DicomViewerWindow/CurrentSliceAnnotationPanel.cpp
    src/DicomViewerWindow/VideoExportController.cpp
    src/DicomViewerWindow/VideoExportDialog.cpp
    src/DicomViewerWindow/RegulatorySplashDialog.cpp
    src/DicomViewerWindow/DicomMainWindow.cpp
    src/DicomViewerWindow/DicomTreeController.cpp
    src/DicomViewerWindow/DicomTreePanel.cpp
    src/DicomViewerWindow/DicomMainWindow.ui
    src/Model/DicomImage.cpp
    src/Services/AnnotationReportService.cpp
    src/Services/CineFrameCache.cpp
    src/Services/MeasurementAnnotationStore.cpp
    src/Services/SeriesPreviewService.cpp
    src/Services/VideoExport/DicomCineFrameProvider.cpp
    src/Services/VideoExport/GStreamerVideoExportService.cpp
    src/DicomViewerWindow/DicomViewportController.cpp
    src/DicomViewerWindow/ViewportSession.cpp
    src/Services/AdvancedSeriesVolumeService.cpp
    src/Services/VolumeBuilder.cpp
    src/Services/VolumeResampleService.cpp
    src/Utilities/WarningDialogService.cpp
    src/Database/SqliteConnection.cpp
    src/Database/SqliteService.cpp
    ${APP_3D_MODEL_SOURCES}
    ${APP_3D_SERVICE_SOURCES}
    ${APP_QML3D_SOURCES}
    ${APP_QML3D_VIEWER_SOURCES}
    ${APP_VTK_VIEWER_SOURCES}
)
set(HEADER_FILES
    src/AI/AiChatTypes.h
    src/AI/GeminiAiAssistantService.h
    src/AI/IAiAssistantService.h
    src/AI/IAiServerClient.h
    src/AI/QtHttpAiServerClient.h
    src/Audit/AuditEvent.h
    src/Audit/AuditService.h
    src/Audit/DicomMetadataAudit.h
    src/Audit/ErrorAuditAdapter.h
    src/Audit/IAuditService.h
    src/Audit/IAuditSink.h
    src/Audit/JsonlAuditSink.h
    src/Audit/NullAuditSink.h
    src/AdvancedViewer/IAdvancedViewerLauncher.h
    src/AdvancedViewer/WidgetsAdvancedViewerLauncher.h
    src/Errors/AppError.h
    src/Errors/AppResult.h
    src/Errors/ErrorCode.h
    src/Errors/ErrorSeverity.h
    src/Errors/IErrorAudit.h
    src/Errors/IErrorPresenter.h
    src/Errors/NullErrorAudit.h
    src/Errors/QtErrorDialogPresenter.h
    src/Utilities/AiServiceSettings.h
    src/Utilities/AppStyle.h
    src/Utilities/AppIcons.h
    src/Utilities/ApplicationPaths.h
    src/Utilities/AppDialogBase.h
    src/Utilities/AiApiKeyDialog.h
    src/Utilities/AiMessageFormatter.h
    src/Utilities/CrashReportService.h
    src/Utilities/DatabaseSettings.h
    src/Utilities/DiagnosticImageRenderer.h
    src/Utilities/MemoryManagementDebug.h
    src/Utilities/Math3D.h
    src/Utilities/IAppConfigService.h
    src/Utilities/IWarningDialogService.h
    src/Utilities/LoadingDialog.h
    src/Utilities/QSettingsAppConfigService.h
    src/Utilities/VolumeValidationSettings.h
    src/ViewerOverlays/CrosshairOverlayWidget.h
    src/ViewerTools/WindowLevelPreset.h
    src/Model/MedicalImage.h
    src/Model/AnnotationReportSummary.h
    src/Model/DicomPreviewItem.h
    src/Model/DicomMetadata.h
    src/Model/DicomImage.h
    src/Model/MeasurementAnnotationRecord.h
    src/Model/DicomParameters.h
    src/Model/VolumeConcepts.h
    src/Model/VolumeGeometry.h
    src/Model/IVolumeData.h
    src/Model/VolumeData.h
    src/Model/VolumeBuildResult.h
    src/FileHandling/FileHandling.h
    src/FileHandling/GDCMFileHandling.h
    src/DicomViewerWindow/DicomPreviewPanel.h
    src/DicomViewerWindow/AnnotationReportDock.h
    src/DicomViewerWindow/AnnotationSliceBrowserPanel.h
    src/DicomViewerWindow/AnnotationUiOptions.h
    src/DicomViewerWindow/CurrentSliceAnnotationPanel.h
    src/DicomViewerWindow/VideoExportController.h
    src/DicomViewerWindow/VideoExportDialog.h
    src/DicomViewerWindow/RegulatorySplashDialog.h
    src/DicomViewerWindow/DicomMainWindow.h
    src/DicomViewerWindow/DicomTreeController.h
    src/DicomViewerWindow/DicomTreeItemRoles.h
    src/DicomViewerWindow/DicomTreePanel.h
    src/DicomViewerWindow/DicomViewportController.h
    src/DicomViewerWindow/ViewportSession.h
    src/Services/AdvancedSeriesVolumeService.h
    src/Services/AnnotationReportService.h
    src/Services/CineFrameCache.h
    src/Services/IAnnotationReportService.h
    src/Services/ISeriesPreviewService.h
    src/Services/SeriesPreviewService.h
    src/Services/VideoExport/DicomCineFrameProvider.h
    src/Services/VideoExport/GStreamerVideoExportService.h
    src/Services/VideoExport/IVideoExportService.h
    src/Services/VideoExport/IVideoFrameProvider.h
    src/Services/VideoExport/VideoExportTypes.h
    src/Services/VolumeBuilder.h
    src/Services/MeasurementAnnotationStore.h
    src/Services/VolumeResampleService.h
    src/Utilities/WarningDialog.h
    src/Utilities/WarningDialogService.h
    src/Database/DatabaseConnection.h
    src/Database/SqliteConnection.h
    src/Database/DatabaseService.h
    src/Database/SqliteService.h
    ${APP_3D_MODEL_HEADERS}
    ${APP_3D_SERVICE_HEADERS}
    ${APP_QML3D_HEADERS}
    ${APP_QML3D_VIEWER_HEADERS}
    ${APP_VTK_VIEWER_HEADERS}
)

# ----------------------------
# Qt executable
# ----------------------------
add_executable(${PROJECT_NAME}
    ${SRC_FILES}
    ${HEADER_FILES}
)

# ----------------------------
# Include directories
# ----------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${DICOMVIEWER_GENERATED_DIR}
        ${GDCM_INCLUDE_DIRS}
        /usr/include/gdcm-3.0
)

# ----------------------------
# Link libraries
# ----------------------------
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Sql
        Qt6::Concurrent
        Qt6::Network
        PkgConfig::GSTREAMER_CORE
        PkgConfig::GSTREAMER_APP
    	${GDCM_LIBRARIES}
        gdcmCommon
        gdcmMSFF
        gdcmIOD
        gdcmDSED
)

if(UNIX AND NOT APPLE)
    # The local development setup can use extracted dependency packages under
    # the build tree. DT_RPATH is intentionally transitive, unlike DT_RUNPATH,
    # so indirect local dependencies such as GDCM sibling libraries resolve
    # without requiring LD_LIBRARY_PATH.
    target_link_options(${PROJECT_NAME} PRIVATE
        "-Wl,--disable-new-dtags"
        "-Wl,-export-dynamic"
    )
endif()

if(DICOMVIEWER_ENABLE_VTK)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${VTK_LIBRARIES})
    target_compile_definitions(${PROJECT_NAME} PRIVATE DICOMVIEWER_ENABLE_VTK=1)

    vtk_module_autoinit(
        TARGETS ${PROJECT_NAME}
        MODULES ${VTK_LIBRARIES}
    )
endif()

if(DICOMVIEWER_ENABLE_QML3D)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            Qt6::Qml
            Qt6::Quick
            Qt6::QuickWidgets
            Qt6::Quick3D
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE DICOMVIEWER_ENABLE_QML3D=1)
endif()

# ----------------------------
# Build output folders
# ----------------------------
# Separate output for Debug and Release
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/Release)

# Optional: also for libraries
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/Debug)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/Release)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/Debug)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/Release)

# ----------------------------
# Platform-specific settings //to do
# ----------------------------
if (WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE ON
    )
    if(MSVC)
        target_link_options(${PROJECT_NAME} PRIVATE /STACK:8388608)
    elseif(MINGW)
        target_link_options(${PROJECT_NAME} PRIVATE -Wl,--stack,8388608)
    endif()
endif()

if (APPLE)
    set(DICOMVIEWER_MACOS_ICON "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/icons/icon.icns")
    set_source_files_properties("${DICOMVIEWER_MACOS_ICON}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(${PROJECT_NAME} PRIVATE "${DICOMVIEWER_MACOS_ICON}")

    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_BUNDLE_NAME "${DICOMVIEWER_DISPLAY_NAME}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${DICOMVIEWER_BUNDLE_IDENTIFIER}"
        MACOSX_BUNDLE_ICON_FILE "icon.icns"
        MACOSX_BUNDLE_BUNDLE_VERSION "${DICOMVIEWER_VERSION_STRING}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${DICOMVIEWER_VERSION_SHORT}"
    )
endif()


# ----------------------------
# Install (optional)
# ----------------------------
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
    )

add_executable(ThreeDPipelineHarness
    tests/ThreeDPipelineHarness.cpp
    src/Audit/DicomMetadataAudit.cpp
    src/Model/DicomImage.cpp
    ${APP_3D_MODEL_SOURCES}
    src/Services/VolumeBuilder.cpp
    ${APP_3D_SERVICE_SOURCES}
    ${APP_3D_MODEL_HEADERS}
    ${APP_3D_SERVICE_HEADERS}
)

target_include_directories(ThreeDPipelineHarness PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GDCM_INCLUDE_DIRS}
    /usr/include/gdcm-3.0
)

target_link_libraries(ThreeDPipelineHarness
    PRIVATE
        Qt6::Core
        Qt6::Gui
)

add_executable(ThreeDSeriesCli
    tests/ThreeDSeriesCli.cpp
    src/Audit/DicomMetadataAudit.cpp
    src/FileHandling/GDCMFileHandling.cpp
    src/Model/DicomImage.cpp
    ${APP_3D_MODEL_SOURCES}
    src/Services/VolumeBuilder.cpp
    src/Services/ThreeDSeriesBuildService.cpp
    ${APP_3D_SERVICE_SOURCES}
    ${APP_3D_MODEL_HEADERS}
    ${APP_3D_SERVICE_HEADERS}
    src/FileHandling/FileHandling.h
    src/FileHandling/GDCMFileHandling.h
)

target_include_directories(ThreeDSeriesCli PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GDCM_INCLUDE_DIRS}
    /usr/include/gdcm-3.0
)

target_link_libraries(ThreeDSeriesCli
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        ${GDCM_LIBRARIES}
        gdcmCommon
        gdcmMSFF
        gdcmIOD
        gdcmDSED
)

add_executable(SqliteServiceSmokeTest
    tests/SqliteServiceSmokeTest.cpp
    src/Services/AnnotationReportService.cpp
    src/Database/SqliteConnection.cpp
    src/Database/SqliteService.cpp
    src/Model/DicomImage.cpp
    src/Database/DatabaseService.h
    src/Database/SqliteConnection.h
    src/Database/SqliteService.h
    src/Model/DicomImage.h
    src/Model/DicomParameters.h
    src/Model/MeasurementAnnotationRecord.h
    src/Model/AnnotationReportSummary.h
    src/Services/AnnotationReportService.h
    src/Services/IAnnotationReportService.h
)

add_executable(FolderImportDbSmokeTest
    tests/FolderImportDbSmokeTest.cpp
    src/FileHandling/GDCMFileHandling.cpp
    src/Database/SqliteConnection.cpp
    src/Database/SqliteService.cpp
    src/Model/DicomImage.cpp
    src/FileHandling/FileHandling.h
    src/FileHandling/GDCMFileHandling.h
    src/Database/DatabaseConnection.h
    src/Database/DatabaseService.h
    src/Database/SqliteConnection.h
    src/Database/SqliteService.h
    src/Model/DicomImage.h
    src/Model/DicomParameters.h
    src/Model/MeasurementAnnotationRecord.h
    src/Model/AnnotationReportSummary.h
    src/Utilities/DatabaseSettings.h
)

add_executable(VideoExportSmokeTest
    tests/VideoExportSmokeTest.cpp
    src/Services/VideoExport/GStreamerVideoExportService.cpp
    src/Services/VideoExport/GStreamerVideoExportService.h
    src/Services/VideoExport/IVideoExportService.h
    src/Services/VideoExport/IVideoFrameProvider.h
    src/Services/VideoExport/VideoExportTypes.h
)

target_include_directories(VideoExportSmokeTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(VideoExportSmokeTest
    PRIVATE
        Qt6::Core
        Qt6::Gui
        PkgConfig::GSTREAMER_CORE
        PkgConfig::GSTREAMER_APP
)

target_include_directories(SqliteServiceSmokeTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(SqliteServiceSmokeTest
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Sql
)

target_include_directories(FolderImportDbSmokeTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GDCM_INCLUDE_DIRS}
    /usr/include/gdcm-3.0
)

target_link_libraries(FolderImportDbSmokeTest
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Sql
        Qt6::Concurrent
        ${GDCM_LIBRARIES}
        gdcmCommon
        gdcmMSFF
        gdcmIOD
        gdcmDSED
)

if(BUILD_TESTING)
    add_test(NAME SqliteServiceSmokeTest COMMAND SqliteServiceSmokeTest)
    set_tests_properties(SqliteServiceSmokeTest PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
    add_test(NAME VideoExportSmokeTest COMMAND VideoExportSmokeTest)
endif()
