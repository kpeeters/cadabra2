# Dockerfile to build a docker container with Cadabra running as a Jupyter
# kernel inside JupyterLab.
#
# Based on https://github.com/fjebaker/cadabra-resources
#      and https://hub.docker.com/r/flavleger/cadabra
#
#
# Build with
#
#    docker build . -t cadabra2-jupyter
#
# Then run with
#
#    docker run -p 9057:9057 -v $(pwd)/notebooks:/home/cadabra cadabra2-jupyter

FROM python:3.11-slim-bookworm AS builder
USER root
ARG DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install -y \
    git cmake pkgconf libpython3-dev python3-dev g++ \
    libgmp3-dev libglibmm-2.4-dev libboost-all-dev libssl-dev libgmp-dev libsqlite3-dev uuid-dev  \
    python3-matplotlib python3-mpmath python3-sympy python3-gmpy2 python3-pip
RUN pip install jupyter
RUN git clone -b "master" --depth 1 https://github.com/kpeeters/cadabra2 && \
    mkdir -p cadabra2/build && cd cadabra2/build && \
    cmake .. -DENABLE_FRONTEND=&& \
    make -j `nproc` && \
    make install

FROM python:3.11-slim-bookworm
USER root
ARG DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install -y \
    libgmp10 \
    libgmpxx4ldbl \
    libsqlite3-0 \
    python3 \
    python3-gmpy2 \
    python3-pip \
    uuid \
 && rm -rf /var/lib/apt/lists/*
RUN pip install jupyter
COPY --from=builder /usr/local/bin/cadabra* /usr/local/bin/
COPY --from=builder /usr/local/lib/python3.11/site-packages /usr/local/lib/python3.11/site-packages
COPY --from=builder /usr/local/share/cadabra2 /usr/local/share/cadabra2
COPY --from=builder /usr/local/share/jupyter/kernels/cadabra2 /usr/local/share/jupyter/kernels/cadabra2

# Cleanup things not necessary for running the container.
RUN apt-get remove -y gcc && apt-get autoremove -y

EXPOSE 9057
WORKDIR /home/cadabra
COPY docker/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh", "--port=9057"]
