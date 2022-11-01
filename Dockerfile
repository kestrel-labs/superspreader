# syntax=docker/dockerfile:1.2
FROM ubuntu:22.04 AS upstream
ARG REPO

# prevent interactive messages in apt install
ARG DEBIAN_FRONTEND=noninteractive

# install tools for building
RUN --mount=type=cache,target=/var/cache/apt,id=apt \
    apt update \
    && apt upgrade -y \
    && apt install -q -y --no-install-recommends \
    build-essential \
    clang-format-14 \
    cmake \
    curl \
    ffmpeg \
    lld \
    mesa-utils \
    python3-pip \
    python3-tk \
    && rm -rf /var/lib/apt/lists/*

RUN python3 -m pip install --upgrade pip setuptools \
    && python3 -m pip install pyserial

RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=/usr/local/bin sh

RUN arduino-cli config init \
    && arduino-cli config add board_manager.additional_urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json \
    && arduino-cli core update-index \
    && arduino-cli core install esp32:esp32

COPY ./scripts/requirements.txt .
RUN pip install -r requirements.txt

# Build deployment image
# ie install more dependencies and create our binary
FROM upstream AS deployment
# this doesn't currently build binaries for deployment but does check that
# everything will build together
WORKDIR /ws
# build arduino code

FROM upstream AS linting
RUN python3 -m pip install --upgrade pip setuptools \
    && python3 -m pip install black pre-commit
WORKDIR /ws/src/${REPO}

# build development environment
FROM linting AS development

# install development tools
RUN --mount=type=cache,target=/var/cache/apt,id=apt \
    apt update \
    && apt install -q -y --no-install-recommends \
    bash-completion \
    ccache \
    gdb \
    git \
    ssh-client \
    sudo \
    tmux \
    vim \
    xterm \
    && rm -rf /var/lib/apt/lists/*

ARG UID
ARG GID
ARG USER

# fail early if args are missing
RUN if [ -z "$USER" ]; then echo '\nERROR: USER not set. Run \n\n \texport USER=$(whoami) \n\n on host before building Dockerfile.\n'; exit 1; fi
RUN if [ -z "$UID" ]; then echo '\nERROR: UID not set. Run \n\n \texport UID=$(id -u) \n\n on host before building Dockerfile.\n'; exit 1; fi
RUN if [ -z "$GID" ]; then echo '\nERROR: GID not set. Run \n\n \texport GID=$(id -g) \n\n on host before building Dockerfile.\n'; exit 1; fi

# add the user so conan installs in the correct home directory, even though
# when we log into the container parts of this will be mapped over
RUN groupadd --gid ${GID} ${USER} \
    && useradd --no-log-init --uid ${UID} --gid ${GID} -m ${USER} --groups sudo
# grab the conanfile for installation
WORKDIR /home/${USER}/ws
RUN chown -R ${UID}:${GID} /home/${USER}
USER ${USER}

RUN arduino-cli config init \
    && arduino-cli config add board_manager.additional_urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json \
    && arduino-cli core update-index \
    && arduino-cli core install esp32:esp32
