
FROM debian:testing

# Needed by qmake, even if we do not use Qt
ENV QT_SELECT=qt5

# Install generic dependencies
RUN apt-get update && apt-get install -y build-essential pkg-config qt5-qmake wget

# Install application dependencies
RUN apt-get install -y libz3-dev libmicrohttpd-dev libcrypto++-dev libboost-system-dev libboost-filesystem-dev libboost-serialization-dev

# Build the application
WORKDIR /srv/mmpp
ADD . /srv/mmpp
RUN mkdir build && cd build && qmake .. && make

# Include a copy of set.mm
RUN mkdir /srv/set.mm && wget https://raw.githubusercontent.com/giomasce/set.mm/master/set.mm -O set.mm

# How to run the application
EXPOSE 8888
CMD ./build/webmmpp
