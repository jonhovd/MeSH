#!/bin/sh

# Fjern elasticsearch 1.x og WT 3.3.3
sudo apt-get remove --purge elasticsearch libwthttp-dev libwt-common libwt-dev libwt38 libwthttp38
sudo apt-get -y autoremove
sudo rm /etc/apt/sources.list.d/elasticsearch.list
sudo apt-get update
sudo apt-get dist-upgrade

# Installer elasticsearch 2.4.5
wget -qO - https://packages.elastic.co/GPG-KEY-elasticsearch | sudo apt-key add - &&
echo "deb https://packages.elastic.co/elasticsearch/2.x/debian stable main" | sudo tee -a /etc/apt/sources.list.d/elasticsearch-2.x.list &&
sudo apt-get install apt-transport-https &&
sudo apt-get update &&
sudo apt-get install elasticsearch g++ git libxml2-dev libboost-all-dev make &&

# Last ned og kompiler Wt
cd .. &&
wget https://github.com/emweb/wt/archive/4.0.3.tar.gz &&
gunzip 4.0.3.tar.gz &&
tar xf 4.0.3.tar &&
rm 4.0.3.tar &&
ln -s wt-4.0.3 wt &&
cd wt &&
mkdir build &&
cd build &&
sudo apt-get install cmake &&
cmake ../ -DWT_CPP_11_MODE:STRING="-std=c++11" &&
make -j2 &&
sudo make install &&
sudo ldconfig

# Oppdater med ny Wt-lokasjon
cd ../../MeSH/ &&
rm ./MeSHWeb/resources &&
ln -s /usr/local/share/Wt/resources ./MeSHWeb/resources

# Kompiler MeSH
git pull &&
cd ./MeSHImport/cpp-elasticsearch/ &&
git pull &&
cd .. &&
make clean &&
make -j2 &&
cd ../MeSHWeb/ &&
make clean &&
make -j2 &&
cd ../ParseTreeStructure/ &&
make clean &&
make -j2 &&
cd .. &&
echo "Ferdig."
