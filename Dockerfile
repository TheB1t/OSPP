FROM debian:12-slim

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        ca-certificates \
        pkg-config \
        sudo \
        make \
        cmake \
        binutils \
        gcc-12 g++-12 \
        grub-common grub-pc-bin \
        git \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 100 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-12 100

RUN apt-get install -y --no-install-recommends \
        qemu-system \
        qemu-system-x86 \
        kpartx \
        parted \
        grub-pc

RUN gcc --version && g++ --version && qemu-system-x86_64 --version && cmake --version

WORKDIR /work
VOLUME ["/work"]

CMD ["/bin/bash"]