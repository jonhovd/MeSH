#!/bin/sh
sudo apt-get -y update &&
sudo apt-get -y install g++ libssl-dev libxml2-dev libboost-all-dev cmake make &&
mkdir -p ~/projects/ &&
cd ~/projects/ &&
wget https://github.com/emweb/wt/archive/4.5.0.tar.gz &&
gunzip 4.5.0.tar.gz &&
tar xf 4.5.0.tar &&
rm 4.5.0.tar &&
ln -s wt-4.5.0 wt &&
cd wt &&
mkdir build &&
cd build &&
#cmake ../ -DENABLE_MSSQLSERVER:BOOL=OFF -DENABLE_MYSQL:BOOL=OFF -DENABLE_FIREBIRD:BOOL=OFF -DENABLE_POSTGRES:BOOL=OFF -DENABLE_SQLITE:BOOL=OFF -DENABLE_LIBWTDBO:BOOL=OFF &&
cmake ../ -DENABLE_LIBWTDBO:BOOL=OFF &&
make -j2 &&
sudo make install &&
sudo ldconfig
