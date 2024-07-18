FROM ubuntu:22.04 AS builder
USER root
ARG DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install -y \
    git cmake libpython3-dev python3-dev g++ \
    libgmp3-dev libgtkmm-3.0-dev libboost-all-dev libgmp-dev libsqlite3-dev uuid-dev dvipng \
    python3-matplotlib python3-mpmath python3-sympy python3-gmpy2 python3-pip
RUN pip install jupyter
RUN git clone https://github.com/kpeeters/cadabra2 && \
    mkdir -p cadabra2/build && cd cadabra2/build && \
    cmake .. && \
    make && \
    make install

FROM ubuntu:22.04
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
COPY --from=builder /usr/local/lib/python3.10/dist-packages /usr/local/lib/python3.10/dist-packages
COPY --from=builder /usr/local/share/cadabra2 /usr/local/share/cadabra2
COPY --from=builder /usr/local/share/jupyter/kernels/cadabra2 /usr/local/share/jupyter/kernels/cadabra2
ENV PYTHONPATH=${PYTHONPATH}:/usr/local/lib/python3.10/dist-packages
EXPOSE 8888
WORKDIR /home/cadabra
ENTRYPOINT ["jupyter", "notebook", "--ip=0.0.0.0", "--allow-root"]
