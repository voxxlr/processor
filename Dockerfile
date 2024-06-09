FROM ubuntu as dev

ENV DEBIAN_FRONTEND noninteractive

RUN apt update
RUN apt install -y apt-utils
RUN apt install -y build-essential

# install cmake 
RUN apt install -y wget
#RUN wget -q https://github.com/Kitware/CMake/releases/download/v3.21.1/cmake-3.21.1-linux-x86_64.sh -O cmake.sh
RUN wget https://github.com/Kitware/CMake/releases/download/v3.18.2/cmake-3.18.2-Linux-x86_64.sh -O cmake.sh
RUN sh cmake.sh --prefix=/usr/local/ --exclude-subdir

RUN apt update
RUN apt install -y python3
RUN apt install -y python3-pip
RUN apt install -y python3-yaml

RUN apt install -y libboost-log-dev
RUN apt install -y proj-data
RUN apt install -y libpng-dev
RUN apt install -y libjpeg-dev
RUN apt install -y libgeotiff-dev
RUN apt install -y libglm-dev
RUN apt install -y libxerces-c-dev
RUN apt install -y liblaszip-dev
RUN export PROJ_LIB=/usr/share/proj

FROM dev as deploy

COPY ./src /root/processor/src
COPY ./CMakeLists.txt /root/processor/
COPY ./process.py /root/processor/

WORKDIR /root/processor/build
RUN cmake .. -DBoost_NO_WARN_NEW_VERSIONS=1
RUN make

CMD ["python3", "/root/processor/process.py", "/root/processor/data"]
