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
        curl \
        cmake \
        lld \
        python3-pip \
        mesa-utils \
        ffmpeg \
    && rm -rf /var/lib/apt/lists/*

RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=/usr/local/bin sh

# Build deployment image
# ie install more dependencies and create our binary
FROM upstream AS deployment
# this doesn't currently build binaries for deployment but does check that
# everything will build together
WORKDIR /ws
# build arduino code

FROM upstream AS linting
WORKDIR /ws/src/${REPO}

# build development environment
FROM linting AS development

ARG UID
ARG GID
ARG USER

# fail early if args are missing
RUN if [ -z "$USER" ]; then echo '\nERROR: USER not set. Run \n\n \texport USER=$(whoami) \n\n on host before building Dockerfile.\n'; exit 1; fi
RUN if [ -z "$UID" ]; then echo '\nERROR: UID not set. Run \n\n \texport UID=$(id -u) \n\n on host before building Dockerfile.\n'; exit 1; fi
RUN if [ -z "$GID" ]; then echo '\nERROR: GID not set. Run \n\n \texport GID=$(id -g) \n\n on host before building Dockerfile.\n'; exit 1; fi

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

# add the user so conan installs in the correct home directory, even though
# when we log into the container parts of this will be mapped over
RUN groupadd --gid ${GID} ${USER} \
    && useradd --no-log-init --uid ${UID} --gid ${GID} -m ${USER} --groups sudo
# grab the conanfile for installation
WORKDIR /home/${USER}/ws
RUN chown -R ${UID}:${GID} /home/${USER}
USER ${USER}
