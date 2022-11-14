# syntax=docker/dockerfile:1.2
FROM ubuntu:22.04 AS development

# prevent interactive messages in apt install
ARG DEBIAN_FRONTEND=noninteractive

# install tools for building
RUN --mount=type=cache,target=/var/cache/apt,id=apt \
    apt update \
    && apt upgrade -y \
    && apt install -q -y --no-install-recommends \
    bash-completion \
    build-essential \
    ccache \
    clang-format-14 \
    cmake \
    curl \
    doxygen \
    ffmpeg \
    gdb \
    git \
    mesa-utils \
    python3-pip \
    python3-tk \
    ssh-client \
    sudo \
    tmux \
    vim \
    xterm

# Install Arduino CLI
RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=/usr/local/bin sh

# Non-root user arguments
ARG UID=1000
ARG GID=$UID
ARG USER=dev

# Add non-root user
RUN groupadd --gid $GID $USER \
    && useradd --uid $GID --gid $UID -m $USER --groups sudo \
    && echo $USER ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USER \
    && chmod 0440 /etc/sudoers.d/$USER \
    && chown -R ${GID}:${UID} /home/${USER}

USER ${USER}
WORKDIR /home/${USER}
ENV SHELL /bin/bash
ENTRYPOINT []

# Install Python tooling
COPY ./scripts/requirements.txt .
RUN python3 -m pip install --upgrade pip setuptools \
    && python3 -m pip install pyserial black pre-commit \
    && python3 -m pip install -r requirements.txt

# Install Rust
RUN curl https://sh.rustup.rs -sSf | bash -s -- -y
RUN . $HOME/.cargo/env \
    && rustup install stable \
    && rustup default stable \
    && rustup component add rustfmt clippy

# Install Arduino toolchain for esp32
RUN arduino-cli config init \
    && arduino-cli config add board_manager.additional_urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json \
    && arduino-cli core update-index \
    && arduino-cli core install esp32:esp32
