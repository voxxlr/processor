FROM ubuntu:20.04

ADD . /root

ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update
RUN apt-get install -y apt-utils

# install c++ compilers
RUN apt-get install -y build-essential

# install cmake 
RUN apt-get install -y wget
#RUN wget -q https://github.com/Kitware/CMake/releases/download/v3.21.1/cmake-3.21.1-linux-x86_64.sh -O cmake.sh
RUN wget https://github.com/Kitware/CMake/releases/download/v3.18.2/cmake-3.18.2-Linux-x86_64.sh -O cmake.sh
RUN sh cmake.sh --prefix=/usr/local/ --exclude-subdir

# install vcpkg
WORKDIR /root
RUN apt-get install -y curl zip unzip tar
RUN apt-get install -y git
RUN apt-get install -y pkg-config
RUN git clone https://github.com/microsoft/vcpkg
RUN ./vcpkg/bootstrap-vcpkg.sh

# build the processor
WORKDIR /root/build
RUN cmake .. -DCMAKE_TOOLCHAIN_FILE=/root/vcpkg/scripts/buildsystems/vcpkg.cmake -DBoost_NO_WARN_NEW_VERSIONS=1
RUN make


FROM python:3.9
#FROM ubuntu:20.04  

#RUN apt-get update
#RUN apt-get install -y python3
#RUN apt-get install -y python3-pip
RUN pip3 install pyyaml

COPY --from=0 /root/process.py /root/
COPY --from=0 /root/bin /root/bin

WORKDIR /root

CMD ["python3", "/root/process.py", "/root/data"]
