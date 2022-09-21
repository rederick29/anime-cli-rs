ARG VARIANT="kinetic"
FROM ubuntu

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
     && apt-get -y install --no-install-recommends \
     curl lld lldb clangd clang build-essential git btop \
     ca-certificates pkg-config libtorrent-rasterbar-dev \
     libtorrent-rasterbar2.0 libtorrent-rasterbar-doc

RUN update-ca-certificates
RUN useradd -m -G adm,users -s /bin/bash vscode
