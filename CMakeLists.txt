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

# ----------------------------
# Find dependencies
# ----------------------------
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql Concurrent Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Sql Concurrent Network)
message(STATUS "Qt major version: ${QT_VERSION_MAJOR}")

find_package(GDCM REQUIRED)
message(STATUS "GDCM")

find_package(OpenCV 4.6 REQUIRED core imgproc imgcodecs)
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
 
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
    src/VTKwidget/VTKwidget.cpp
    src/Database/PostgreConnection.cpp
    src/Database/PostgreService.cpp
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
    src/VTKwidget/VTKwidget.h
    src/Database/DatabaseConnection.h
    src/Database/PostgreConnection.h
    src/Database/DatabaseService.h
    src/Database/PostgreService.h
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
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Sql
        Qt${QT_VERSION_MAJOR}::Concurrent
        Qt${QT_VERSION_MAJOR}::Network
        ${OpenCV_LIBS}
    	${GDCM_LIBRARIES}
        gdcmCommon
        gdcmMSFF
        gdcmIOD
        gdcmDSED
)

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
