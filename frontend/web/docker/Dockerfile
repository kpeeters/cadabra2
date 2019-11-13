# our base image
FROM continuumio/miniconda3
LABEL maintainer="info@cadabra.science"

ADD 2.2.7.tar.gz /tmp
RUN ls -laR /tmp
RUN apt update && apt install -y cmake g++ pkg-config \
                      libpcre3 libpcre3-dev libgmp3-dev \
                      libglibmm-2.4-dev \
                      libboost-system-dev \
                      libboost-program-options-dev \
                      libboost-date-time-dev \
                      libboost-filesystem-dev \
                      libgmp-dev libsqlite3-dev uuid-dev  
RUN cd /tmp/cadabra2-2.2.7 && mkdir build && cd build && cmake -DENABLE_FRONTEND=OFF .. && make -j4 && make install
RUN rm -Rf /tmp/cadabra2-2.2.7
RUN apt purge         cmake g++ pkg-config \
                      libpcre3 libpcre3-dev libgmp3-dev \
                      libglibmm-2.4-dev \
                      libboost-system-dev \
                      libboost-program-options-dev \
                      libboost-date-time-dev \
                      libboost-filesystem-dev \
                      libgmp-dev libsqlite3-dev uuid-dev 

#COPY cadabra-server /usr/bin/cadabra-server
 
#CMD ["/usr/local/bin/cadabra-server", "-D", "FOREGROUND"]
#EXPOSE 80
 