FROM ubuntu:20.04

WORKDIR /gprc_service

COPY src/ include/ scripts/ ./

ENV MY_INSTALL_DIR=${WORKDIR}/.local
ENV PATH=${MY_INSTALL_DIR}/bin:${PATH}
RUN mkdir -p ${MY_INSTALL_DIR}

RUN apt update && apt install -y cmake
RUN apt install -y build-essential autoconf libtool pkg-config git

RUN git clone --recurse-submodules -b v1.58.0 --depth 1 --shallow-submodules \
    https://github.com/grpc/grpc
RUN cd grpc && \
    mkdir -p cmake/build && \
    pushd cmake/build && \
    cmake -DgRPC_INSTALL=ON \
        -DgRPC_BUILD_TESTS=OFF \
        -DCMAKE_INSTALL_PREFIX=${MY_INSTALL_DIR} \
        ../.. && \
    make -j4 && \
    make install && \
    popd