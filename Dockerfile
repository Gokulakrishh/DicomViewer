FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
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
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN cmake -S . -B build-ci -G Ninja -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build-ci --parallel

CMD ["cmake", "--build", "build-ci", "--parallel"]
