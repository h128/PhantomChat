# [Choice] noble (24.04)
ARG VARIANT="noble"
FROM ubuntu:${VARIANT}

# Restate the variant to use it later on in the llvm and cmake installations
ARG VARIANT

# Install necessary packages available from standard repos
RUN apt-get update -qq && export DEBIAN_FRONTEND=noninteractive && \
    apt-get install -y --no-install-recommends \
    software-properties-common apt-utils file zip \
    openssh-client socat rsync \
    make ninja-build git \
    ca-certificates

# GCC 13 is available in Ubuntu 24.04's default repos — no PPA needed
ARG GCC_VER="13"
RUN apt-get update -qq && export DEBIAN_FRONTEND=noninteractive && \
    apt-get install -y --no-install-recommends \
    gcc-${GCC_VER} g++-${GCC_VER} libstdc++-${GCC_VER}-dev gdb

# Set gcc-${GCC_VER} as default gcc
RUN update-alternatives --install /usr/bin/gcc gcc $(which gcc-${GCC_VER}) 100
RUN update-alternatives --install /usr/bin/g++ g++ $(which g++-${GCC_VER}) 100

# Install the default clang/LLVM from Ubuntu 24.04's repos (clang-18).
# No external apt source needed — clang, lldb, lld, clangd, clang-tidy and
# the llvm/libclang dev headers are all available in the default repos.
ARG LLVM_VER="18"
RUN apt-get update -qq && export DEBIAN_FRONTEND=noninteractive && \
    apt-get install -y --no-install-recommends \
    clang-${LLVM_VER} lldb-${LLVM_VER} lld-${LLVM_VER} clangd-${LLVM_VER} \
    llvm-${LLVM_VER}-dev libclang-${LLVM_VER}-dev clang-tidy-${LLVM_VER}

# Set the default clang-tidy, so CMake can find it
RUN update-alternatives --install /usr/bin/clang-tidy clang-tidy $(which clang-tidy-${LLVM_VER}) 1

# Set clang-${LLVM_VER} as default clang
RUN update-alternatives --install /usr/bin/clang clang $(which clang-${LLVM_VER}) 100
RUN update-alternatives --install /usr/bin/clang++ clang++ $(which clang++-${LLVM_VER}) 100

# cmake 3.28 is available in Ubuntu 24.04's default repos — no Kitware PPA needed
RUN apt-get update -qq && export DEBIAN_FRONTEND=noninteractive && \
    apt-get install -y --no-install-recommends cmake cmake-curses-gui curl

# Install editors
RUN apt-get update -qq && export DEBIAN_FRONTEND=noninteractive && \
    apt-get install -y --no-install-recommends \
    neovim emacs nano

# Install optional dependencies
RUN apt-get update -qq && export DEBIAN_FRONTEND=noninteractive && \
    apt-get install -y --no-install-recommends \
    doxygen graphviz ccache cppcheck libssl-dev

# Install include-what-you-use
# Branch clang_18 matches the default Ubuntu 24.04 clang version
ENV IWYU=/home/iwyu
ENV IWYU_BUILD=${IWYU}/build
ENV IWYU_SRC=${IWYU}/include-what-you-use
RUN mkdir -p ${IWYU_BUILD} && \
    git clone --branch clang_${LLVM_VER} \
    https://github.com/include-what-you-use/include-what-you-use.git \
    ${IWYU_SRC}
RUN CC=gcc CXX=g++ cmake -S ${IWYU_SRC} \
    -B ${IWYU_BUILD} \
    -G "Unix Makefiles" \
    -DCMAKE_PREFIX_PATH=/usr/lib/llvm-${LLVM_VER} \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 && \
    cmake --build ${IWYU_BUILD} -j && \
    cmake --install ${IWYU_BUILD}

# Per https://github.com/include-what-you-use/include-what-you-use#how-to-install:
# `You need to copy the Clang include directory to the expected location before
#  running (similarly, use include-what-you-use -print-resource-dir to learn
#  exactly where IWYU wants the headers).`
RUN mkdir -p $(include-what-you-use -print-resource-dir 2>/dev/null)
RUN ln -s $(readlink -f /usr/lib/clang/${LLVM_VER}/include) \
    $(include-what-you-use -print-resource-dir 2>/dev/null)/include

## Cleanup cached apt data we don't need anymore
RUN apt-get autoremove -y && apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Allow the user to set compiler defaults
ARG USE_CLANG
# if --build-arg USE_CLANG=1, set CC to 'clang' or set to null otherwise.
ENV CC=${USE_CLANG:+"clang"}
ENV CXX=${USE_CLANG:+"clang++"}
# if CC is null, set it to 'gcc' (or leave as is otherwise).
ENV CC=${CC:-"gcc"}
ENV CXX=${CXX:-"g++"}

# Include project
ADD . /workspaces/cpp_starter_project
WORKDIR /workspaces/cpp_starter_project

CMD ["/bin/bash"]