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
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Sql)
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
    src/Config/QSettingsDatabaseConfigService.cpp
    src/FileHandling/GDCMFileHandling.cpp
    src/DicomViewerWindow/DicomMainWindow.cpp
    src/DicomViewerWindow/DicomGraphicsView.cpp
    src/DicomViewerWindow/DicomMainWindow.ui
    src/UI/WarningDialogService.cpp
    src/VTKwidget/VTKwidget.cpp
    src/Database/PostgreConnection.cpp
    src/Database/PostgreService.cpp
)
set(HEADER_FILES
    src/Config/DatabaseSettings.h
    src/Config/QSettingsDatabaseConfigService.h
    src/Model/MedicalImage.h
    src/Model/DicomImage.h
    src/Model/DicomParameters.h
    src/FileHandling/FileHandling.h
    src/FileHandling/GDCMFileHandling.h
    src/DicomViewerWindow/DicomMainWindow.h
    src/DicomViewerWindow/DicomGraphicsView.h
    src/UI/WarningDialogService.h
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
endif()


# ----------------------------
# Install (optional)
# ----------------------------
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    )
