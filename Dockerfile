
FROM debian:testing

# Needed by qmake, even if we do not use Qt
ENV QT_SELECT=qt5

# Install generic dependencies
RUN apt-get update && apt-get install -y build-essential pkg-config qt5-qmake wget

# Install C++ application dependencies
RUN apt-get update && apt-get install -y libz3-dev libmicrohttpd-dev libboost-all-dev

# Install TypeScript application dependencies
RUN apt-get update && apt-get install -y node-typescript

# Include a copy of set.mm
RUN mkdir /srv/set.mm && cd /srv/set.mm && wget https://raw.githubusercontent.com/giomasce/set.mm/develop/set.mm -O set.mm

# Copy repository in the container
WORKDIR /srv/mmpp
ADD . /srv/mmpp

# Build the C++ application
RUN mkdir build && cd build && qmake .. && make

# Build the TypeScript application
# There is currently a bug (tsc invokes "node", but nodejs only provides "nodejs"), so we explicitly call "nodejs"
RUN nodejs /usr/bin/tsc -p resources/static/ts
RUN cd resources && rm -f set.mm && ln -s ../../set.mm/set.mm set.mm

# How to run the application
EXPOSE 8888
CMD ./build/mmpp webmmpp_docker
