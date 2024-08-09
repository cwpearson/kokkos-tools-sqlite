FROM debian:buster-20181112-slim

RUN apt-get --allow-unauthenticated --allow-insecure-repositories update \
 && apt-get --allow-unauthenticated install -y --no-install-suggests --no-install-recommends gnupg

RUN apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 648ACFD622F3D138 \
 && apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 0E98404D386FA1D9 \
 && apt-key adv --keyserver keyserver.ubuntu.com --recv-keys DCC9EFBF77E11517 \
 && apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 112695A0E562B32A \
 && apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 54404762BBB6E853
   
RUN apt-get update \
 && apt-get upgrade -y \
 && apt-get install -y --no-install-suggests --no-install-recommends \
 ca-certificates make g++ libsqlite3-dev openssl sqlite3 wget

RUN wget -L https://github.com/Kitware/CMake/releases/download/v3.30.2/cmake-3.30.2.tar.gz \
 && tar -xf cmake-3.30.2.tar.gz \
 && (cd cmake-3.30.2; ./bootstrap && make -j $(nproc) && make install)

ADD . /src

RUN cmake -S /src -B $HOME/build

RUN cmake --build $HOME/build

RUN ldd $HOME/build/libkts.so
