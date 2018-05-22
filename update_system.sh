#!/bin/sh

git pull &&

# Kopier til init.d
sudo cp ./MeSHWeb/etc_initd_MeSHWeb.sh /etc/init.d/MeSHWeb.sh &&
sudo chmod 755 /etc/init.d/MeSHWeb.sh &&

# Oppdater cpp-elasticsearch
cd ./MeSHImport/cpp-elasticsearch/ &&
git pull &&

# Kompiler MeSHImport
cd .. &&
make clean &&
make -j2 &&

# Kompiler MeSHWeb
cd ../MeSHWeb/ &&
make clean &&
make -j2 &&
sudo make install &&

# Kompiler ParseTreeStructure
cd ../ParseTreeStructure/ &&
make clean &&
make -j2 &&
cd ..
