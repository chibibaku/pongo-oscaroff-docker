# syntax=docker/dockerfile:1

FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        gnupg \
        git \
        make \
        clang \
        llvm \
        llvm-10-dev \
        build-essential \
        texinfo \
        pkg-config \
        xz-utils \
        file \
    && rm -rf /var/lib/apt/lists/*

# PongoOS' documented Linux linker/tooling packages.
RUN mkdir -p /usr/share/keyrings \
    && curl -fsSL https://assets.checkra.in/debian/archive.key \
       | gpg --dearmor -o /usr/share/keyrings/checkra1n.gpg \
    && printf '%s\n' \
       'deb [signed-by=/usr/share/keyrings/checkra1n.gpg] https://assets.checkra.in/debian /' \
       > /etc/apt/sources.list.d/checkra1n.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
         ld64 \
         cctools-strip \
    && rm -rf /var/lib/apt/lists/*

ARG PONGO_REPO=https://github.com/palera1n/PongoOS.git
ARG PONGO_REF=iOS15

RUN git clone --recursive --branch "${PONGO_REF}" "${PONGO_REPO}" /opt/PongoOS \
    && git -C /opt/PongoOS submodule update --init --recursive

WORKDIR /opt/PongoOS

# Populate the Darwin/newlib include tree used by the module Makefile.
RUN make -C newlib all \
    && test -f newlib/aarch64-none-darwin/include/sys/cdefs.h

# Keep the module build flags in sync with this PongoOS branch.
RUN cp -a example/testmodule example/oscaroff \
    && sed -i 's/testmodule/oscaroff/g' example/oscaroff/Makefile

COPY oscaroff/main.c /opt/PongoOS/example/oscaroff/main.c
COPY build-oscaroff.sh /usr/local/bin/build-oscaroff

RUN chmod +x /usr/local/bin/build-oscaroff \
    && command -v clang \
    && command -v ld64 \
    && command -v cctools-strip \
    && test -f /opt/PongoOS/newlib/aarch64-none-darwin/include/sys/cdefs.h \
    && test -f /usr/lib/llvm-10/lib/libLTO.so

ENTRYPOINT ["/usr/local/bin/build-oscaroff"]
