FROM ubuntu:22.04 AS labelmaster
LABEL maintainer="3159890292@qq.com" version="1.0-base" description="labelmaster dev"
ENV DEBIAN_FRONTEND=noninteractive 
ENV TZ=Asia/Shanghai
SHELL ["/bin/bash", "-c"]

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake git wget curl vim \
    qt6-base-dev qt6-tools-dev qt6-tools-dev-tools \
    libgl1-mesa-dev xcb libx11-xcb-dev \
    libopencv-dev \
    libeigen3-dev \ 
    # LLVM install dep
    gnupg lsb-release ca-certificates \ 
    # sudo 和 中文字体 (否则 QT ui不显示)
    sudo fonts-noto-cjk \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

ARG CLANG_VERSION=20

RUN wget -qO - https://apt.llvm.org/llvm-snapshot.gpg.key | gpg --dearmor -o /usr/share/keyrings/llvm-snapshot.gpg && \
    echo "deb [signed-by=/usr/share/keyrings/llvm-snapshot.gpg] http://apt.llvm.org/$(lsb_release -cs)/ llvm-toolchain-$(lsb_release -cs)-${CLANG_VERSION} main" \
      > /etc/apt/sources.list.d/llvm-apt.list && \
    apt-get update && apt-get install -y --no-install-recommends \
      clang-${CLANG_VERSION} \
      clangd-${CLANG_VERSION} \
      clang-format-${CLANG_VERSION} clang-tidy-${CLANG_VERSION} \
      gcc-12 g++-12 \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 50 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-12 50 \
    && update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${CLANG_VERSION} 50 \
    && update-alternatives --install /usr/bin/clangd clangd /usr/bin/clangd-${CLANG_VERSION} 50 \
    && update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-${CLANG_VERSION} 50 \
    && update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-${CLANG_VERSION} 50 \
    && rm -rf /var/lib/apt/lists/*

ARG USERNAME=developer
ARG USER_UID=1000
ARG USER_GID=${USER_UID}
RUN groupadd -g ${USER_GID} ${USERNAME} && \
    useradd -m -u ${USER_UID} -g ${USER_GID} -s /bin/bash ${USERNAME} && \
    echo "${USERNAME}:aaa" | chpasswd && \
    echo "root:aaa" | chpasswd && \
    echo "${USERNAME} ALL=(ALL:ALL) NOPASSWD:ALL" >> /etc/sudoers && \
    gpasswd --add ${USERNAME} dialout && \
    gpasswd --add ${USERNAME} plugdev && \
    gpasswd --add ${USERNAME} sudo

USER ${USERNAME}
WORKDIR /home/${USERNAME}
