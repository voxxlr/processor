## linux

```sh
sudo passwd root

apt-get update
apt-get install wget

wget https://github.com/Kitware/CMake/releases/download/v3.18.2/cmake-3.18.2-Linux-x86_64.sh -O cmake.sh
sh cmake.sh --prefix=/usr/local/ --exclude-subdir

apt-get install build-essential tar zip unzip git 
apt-get install pkg-config

cd ~
git clone https://github.com/microsoft/vcpkg
./vcpkg/bootstrap-vcpkg.sh

// clone this repo
git clone https://jochenstier:###@github.com/voxxlr/test.git processor
cd processor
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=/root/vcpkg/scripts/buildsystems/vcpkg.cmake
make


apt-get -y install python3-pip

python3 -m pip install psutil
python3 -m pip install --upgrade google-cloud-datastore
python3 -m pip install --upgrade Pillow

crontab -e
@reboot /usr/bin/python3 /root/processor/process.py


vi ~/.bashrc
alias reboot="sudo systemctl reboot"

// set open file limit
ulimit -Sn 4096

cd ~/test
cp processor.py ..

```

## windows

py -m pip install psutil

cmake .. -DCMAKE_TOOLCHAIN_FILE=d:/test/vcpkg/scripts/buildsystems/vcpkg.cmake


