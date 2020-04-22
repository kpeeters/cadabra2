# our base image
FROM library/debian
#continuumio/miniconda3
LABEL maintainer="info@cadabra.science"

ADD master.tar.gz /tmp
RUN adduser --disabled-password cadabra && apt update && apt install -y cmake g++ pkg-config \
                      python3-dev python3-sympy \
                      libpcre3 libpcre3-dev libgmp3-dev \
                      libglibmm-2.4-dev \
                      libboost-system-dev \
                      libboost-program-options-dev \
                      libboost-date-time-dev \
                      libboost-filesystem-dev \
                      libgmp-dev libsqlite3-dev uuid-dev && rm -rf /var/lib/apt/lists/* && \
    cd /tmp/cadabra2 && mkdir build && cd build && cmake -DENABLE_FRONTEND=OFF .. && make -j4 && make install && \
    rm -Rf /tmp/cadabra2
# RUN apt remove -y     cmake g++ pkg-config \
#                       libpcre3-dev libgmp3-dev \
#                       libglibmm-2.4-dev \
#                       libboost-system-dev \
#                       libboost-program-options-dev \
#                       libboost-date-time-dev \
#                       libboost-filesystem-dev \
#                       libgmp-dev libsqlite3-dev uuid-dev 

#COPY cadabra-server /usr/bin/cadabra-server

# Start the Cadabra server on port 32768, do not exit process
CMD ["su", "-", "cadabra", "-c", "/usr/local/bin/cadabra-server 32768 0"]

EXPOSE 32768
 