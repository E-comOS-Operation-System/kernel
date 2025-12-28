FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    nasm \
    qemu-system-x86 \
    binutils-x86-64-linux-gnu \
    gcc-x86-64-linux-gnu \
    gdb \
    hexdump \
    vim \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /kernel
COPY . .

EXPOSE 5900

CMD ["/bin/bash"]