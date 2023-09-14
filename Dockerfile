# run docker build --network=host -t ubuntu-grpc-cpp-server .
FROM ubuntu:20.04

WORKDIR /gprc_service

COPY src/ include/ scripts/ ./

ENV MY_INSTALL_DIR=${WORKDIR}/.local
ENV PATH=${MY_INSTALL_DIR}/bin:${PATH}
RUN mkdir -p ${MY_INSTALL_DIR}

RUN apt update && apt install -y cmake
RUN apt install -y build-essential autoconf libtool pkg-config git

# host proxy address
RUN git config --global http.proxy "socks://127.0.0.1:10808"

# RUN ./xray/xray -config ./xray/config.json & 
RUN git clone --recurse-submodules -b v1.58.0 --depth 1 --shallow-submodules \
        https://github.com/grpc/grpc
# COPY grpc/ ./grpc

RUN /bin/bash -c "cd grpc && \
    mkdir -p cmake/build && \
    pushd cmake/build && \
    cmake -DgRPC_INSTALL=ON \
        -DgRPC_BUILD_TESTS=OFF \
        -DCMAKE_INSTALL_PREFIX=${MY_INSTALL_DIR} \
        ../.. && \
    make -j4 && \
    make install && \
    popd"