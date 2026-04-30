FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive
ARG VTK_VERSION=9.5.0

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    curl \
    git \
    ninja-build \
    pkg-config \
    qt6-base-dev \
    qt6-base-dev-tools \
    libqt6sql6-psql \
    libgdcm-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libxkbcommon-x11-0 \
    libxkbcommon-dev \
    libdbus-1-3 \
    libexpat1-dev \
    libfreetype-dev \
    libfontconfig1-dev \
    libdouble-conversion-dev \
    libpng-dev \
    libjpeg-dev \
    libtiff-dev \
    liblz4-dev \
    liblzma-dev \
    zlib1g-dev \
    libeigen3-dev \
    libtbb-dev \
    libxml2-dev \
    libcurl4-openssl-dev \
    libopenjp2-7-dev \
    libogg-dev \
    libtheora-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /tmp/vtk-build

RUN curl -L --retry 3 \
        "https://github.com/Kitware/VTK/archive/refs/tags/v${VTK_VERSION}.tar.gz" \
        -o /tmp/vtk.tar.gz \
    && tar -xf /tmp/vtk.tar.gz -C /tmp \
    && cmake -S "/tmp/VTK-${VTK_VERSION}" -B /tmp/vtk-build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/vtk \
        -DBUILD_SHARED_LIBS=ON \
        -DVTK_QT_VERSION=6 \
        -DVTK_BUILD_TESTING=OFF \
        -DVTK_BUILD_EXAMPLES=OFF \
        -DVTK_MODULE_ENABLE_VTK_CommonCore=YES \
        -DVTK_MODULE_ENABLE_VTK_CommonDataModel=YES \
        -DVTK_MODULE_ENABLE_VTK_FiltersModeling=YES \
        -DVTK_MODULE_ENABLE_VTK_FiltersSources=YES \
        -DVTK_MODULE_ENABLE_VTK_InteractionWidgets=YES \
        -DVTK_MODULE_ENABLE_VTK_GUISupportQt=YES \
        -DVTK_MODULE_ENABLE_VTK_IOImage=YES \
        -DVTK_MODULE_ENABLE_VTK_InteractionImage=YES \
        -DVTK_MODULE_ENABLE_VTK_InteractionStyle=YES \
        -DVTK_MODULE_ENABLE_VTK_RenderingCore=YES \
        -DVTK_MODULE_ENABLE_VTK_RenderingImage=YES \
        -DVTK_MODULE_ENABLE_VTK_RenderingOpenGL2=YES \
        -DVTK_MODULE_ENABLE_VTK_RenderingVolumeOpenGL2=YES \
    && cmake --build /tmp/vtk-build --parallel \
    && cmake --install /tmp/vtk-build \
    && VTK_CONFIG_DIR="$(find /opt/vtk -name VTKConfig.cmake -exec dirname {} \; | head -n 1)" \
    && test -n "$VTK_CONFIG_DIR" \
    && ln -sfn "$VTK_CONFIG_DIR" /opt/vtk/lib/cmake/vtk \
    && rm -rf /tmp/vtk-build /tmp/vtk.tar.gz /tmp/VTK-${VTK_VERSION}

ENV VTK_DIR=/opt/vtk/lib/cmake/vtk
ENV CMAKE_PREFIX_PATH=/opt/vtk/lib/cmake/vtk

WORKDIR /workspace
