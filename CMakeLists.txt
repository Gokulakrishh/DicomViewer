cmake_minimum_required(VERSION 3.21)

project(DicomViewer
    VERSION 1.0
    DESCRIPTION "Dicom Image Viewer (2D/3D)"
    LANGUAGES C CXX
)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
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

option(DICOMVIEWER_ENABLE_VTK "Enable VTK-based 3D volume rendering integration" OFF)

# ----------------------------
# Find dependencies
# ----------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql Concurrent Network)
find_package(Qt6 QUIET COMPONENTS Qml Quick QuickWidgets Quick3D)
message(STATUS "Qt major version: 6")

set(DICOMVIEWER_ENABLE_QML3D OFF)
if(TARGET Qt6::Qml AND TARGET Qt6::Quick AND TARGET Qt6::QuickWidgets AND TARGET Qt6::Quick3D)
    set(DICOMVIEWER_ENABLE_QML3D ON)
    message(STATUS "Legacy QML 3D viewer enabled")
else()
    message(STATUS "Legacy QML 3D viewer disabled: Qt Qml/Quick/QuickWidgets/Quick3D stack not fully available")
endif()

find_package(GDCM REQUIRED)
message(STATUS "GDCM")

find_package(OpenCV 4.6 REQUIRED core imgproc imgcodecs)
message(STATUS "OpenCV version: ${OpenCV_VERSION}")

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
        GUISupportQt
        InteractionStyle
        RenderingCore
        RenderingOpenGL2
        RenderingVolumeOpenGL2
    )
    message(STATUS "VTK version: ${VTK_VERSION}")

    list(APPEND APP_VTK_VIEWER_SOURCES
        src/VTK/Adapters/VtkVolumeAdapter.cpp
        src/VTK/Camera/VtkVolumeCameraController.cpp
        src/VTK/Presets/VtkVolumeRenderPreset.cpp
        src/VTK/Presets/VtkVolumePresetLibrary.cpp
        src/VTK/Viewer/VtkVolumeViewerWindow.cpp
    )

    list(APPEND APP_VTK_VIEWER_HEADERS
        src/VTK/Adapters/VtkVolumeAdapter.h
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
    src/AdvancedViewer/MprViewerWindow.cpp
    src/AdvancedViewer/WidgetsAdvancedViewerLauncher.cpp
    src/Utilities/AppStyle.cpp
    src/Utilities/AppDialogBase.cpp
    src/Utilities/AiApiKeyDialog.cpp
    src/Utilities/LoadingDialog.cpp
    src/Utilities/QSettingsAppConfigService.cpp
    src/Utilities/WarningDialog.cpp
    src/FileHandling/GDCMFileHandling.cpp
    src/DicomViewerWindow/DicomMainWindow.cpp
    src/DicomViewerWindow/DicomGraphicsView.cpp
    src/DicomViewerWindow/DicomMainWindow.ui
    src/Model/DicomImage.cpp
    src/DicomViewerWindow/MeasurementController.cpp
    src/DicomViewerWindow/DicomViewportController.cpp
    src/DicomViewerWindow/DicomRenderService.cpp
    src/DicomViewerWindow/ViewportSession.cpp
    src/Services/VolumeBuilder.cpp
    src/Services/VolumeResampleService.cpp
    src/Services/MprRenderService.cpp
    src/Services/WindowingAnalysisService.cpp
    src/Utilities/WarningDialogService.cpp
    src/Database/PostgreConnection.cpp
    src/Database/PostgreService.cpp
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
    src/AdvancedViewer/IAdvancedViewerLauncher.h
    src/AdvancedViewer/MprViewerWindow.h
    src/AdvancedViewer/WidgetsAdvancedViewerLauncher.h
    src/Utilities/AiServiceSettings.h
    src/Utilities/AppStyle.h
    src/Utilities/AppDialogBase.h
    src/Utilities/AiApiKeyDialog.h
    src/Utilities/DatabaseSettings.h
    src/Utilities/Math3D.h
    src/Utilities/IAppConfigService.h
    src/Utilities/IWarningDialogService.h
    src/Utilities/LoadingDialog.h
    src/Utilities/QSettingsAppConfigService.h
    src/Utilities/VolumeValidationSettings.h
    src/Model/MedicalImage.h
    src/Model/DicomImage.h
    src/Model/DicomParameters.h
    src/Model/VolumeConcepts.h
    src/Model/VolumeGeometry.h
    src/Model/IVolumeData.h
    src/Model/VolumeData.h
    src/FileHandling/FileHandling.h
    src/FileHandling/GDCMFileHandling.h
    src/DicomViewerWindow/DicomMainWindow.h
    src/DicomViewerWindow/DicomGraphicsView.h
    src/DicomViewerWindow/MeasurementController.h
    src/DicomViewerWindow/DicomViewportController.h
    src/DicomViewerWindow/DicomRenderService.h
    src/DicomViewerWindow/ViewportSession.h
    src/Services/VolumeBuilder.h
    src/Services/VolumeResampleService.h
    src/Services/MprRenderService.h
    src/Services/WindowingAnalysisService.h
    src/Utilities/WarningDialog.h
    src/Utilities/WarningDialogService.h
    src/Database/DatabaseConnection.h
    src/Database/PostgreConnection.h
    src/Database/DatabaseService.h
    src/Database/PostgreService.h
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
        ${OpenCV_LIBS}
    	${GDCM_LIBRARIES}
        gdcmCommon
        gdcmMSFF
        gdcmIOD
        gdcmDSED
)

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
endif()

if (APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE ON
    )

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/config.ini
            $<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/MacOS/config.ini
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
    src/Model/DicomImage.cpp
    ${APP_3D_MODEL_SOURCES}
    src/Services/VolumeBuilder.cpp
    ${APP_3D_SERVICE_SOURCES}
    ${APP_3D_MODEL_HEADERS}
    ${APP_3D_SERVICE_HEADERS}
)

target_include_directories(ThreeDPipelineHarness PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    /usr/include/gdcm-3.0
)

target_link_libraries(ThreeDPipelineHarness
    PRIVATE
        Qt6::Core
        Qt6::Gui
)

add_executable(ThreeDSeriesCli
    tests/ThreeDSeriesCli.cpp
    src/FileHandling/GDCMFileHandling.cpp
    src/DicomViewerWindow/DicomRenderService.cpp
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
    /usr/include/gdcm-3.0
)

target_link_libraries(ThreeDSeriesCli
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        ${OpenCV_LIBS}
        ${GDCM_LIBRARIES}
        gdcmCommon
        gdcmMSFF
        gdcmIOD
        gdcmDSED
)
