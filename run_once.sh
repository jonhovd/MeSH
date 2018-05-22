#!/bin/sh

# Installer elasticsearch 2.x
wget -qO - https://packages.elastic.co/GPG-KEY-elasticsearch | sudo apt-key add - &&
echo "deb https://packages.elastic.co/elasticsearch/2.x/debian stable main" | sudo tee -a /etc/apt/sources.list.d/elasticsearch-2.x.list &&
sudo apt-get install apt-transport-https &&
sudo apt-get update &&
sudo apt-get -y install elasticsearch default-jdk g++ git libxml2-dev libboost-all-dev cmake make &&
sudo apt-get dist-upgrade &&

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
cmake ../ -DWT_CPP_11_MODE:STRING="-std=c++11" &&
make -j2 &&
sudo make install &&
sudo ldconfig &&

# Oppdater med ny Wt-lokasjon
cd ../../MeSH/ &&
ln -sf /usr/local/share/Wt/resources ./MeSHWeb/ &&

# Last ned cpp-elasticsearch
cd ./MeSHImport/ &&
git clone https://github.com/QHedgeTech/cpp-elasticsearch.git &&

cd ../MeSHWeb/ &&
ln -sf ../MeSHImport/cpp-elasticsearch &&
cd ../ParseTreeStructure/ &&
ln -sf ../MeSHImport/cpp-elasticsearch &&

sudo mkdir -p /opt/Helsebib/MeSHWeb/ &&
sudo ln -sf /usr/local/share/Wt/resources /opt/Helsebib/MeSHWeb/ &&

./update_system.sh &&

echo "Ferdig."
