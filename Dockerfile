
FROM debian:testing

RUN apt-get update && apt-get install -y build-essential pkg-config qt5-qmake
RUN apt-get install -y libz3-dev libmicrohttpd-dev libcrypto++-dev libboost-system-dev libboost-filesystem-dev libboost-serialization-dev

WORKDIR /srv/mmpp
ADD . /srv/mmpp

# Needed by qmake, even if we do not use Qt
ENV QT_SELECT=qt5

RUN mkdir build && cd build && qmake .. && make
