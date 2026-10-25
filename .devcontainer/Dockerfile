FROM ubuntu:26.04

ENV DEBIAN_FRONTEND=noninteractive

# General build deps
RUN apt-get update && apt-get -y install git cmake ninja-build clang clangd autoconf autoconf-archive automake \
                                         libtool pkg-config python3.14-venv libwayland-dev libxkbcommon-dev \
                                         libegl1-mesa-dev vulkan-validationlayers

# vcpkg
RUN apt-get install curl zip unzip tar
RUN git clone https://github.com/microsoft/vcpkg.git /usr/local/vcpkg \
    && /usr/local/vcpkg/bootstrap-vcpkg.sh
ENV VCPKG_ROOT=/usr/local/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

# opencode
RUN apt-get install -y npm
RUN npm install -g @opencode/cli

# ARG REINSTALL_CMAKE_VERSION_FROM_SOURCE="none"

# Optionally install the cmake for vcpkg
# COPY ./reinstall-cmake.sh /tmp/

# RUN if [ "${REINSTALL_CMAKE_VERSION_FROM_SOURCE}" != "none" ]; then \
#         chmod +x /tmp/reinstall-cmake.sh && /tmp/reinstall-cmake.sh ${REINSTALL_CMAKE_VERSION_FROM_SOURCE}; \
#     fi \
#     && rm -f /tmp/reinstall-cmake.sh

# [Optional] Uncomment this section to install additional vcpkg ports.
# RUN su vscode -c "${VCPKG_ROOT}/vcpkg install <your-port-name-here>"

# [Optional] Uncomment this section to install additional packages.
# RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
#     && apt-get -y install --no-install-recommends <your-package-list-here>
