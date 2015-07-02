#!/bin/sh
wget -qO - https://packages.elasticsearch.org/GPG-KEY-elasticsearch | sudo apt-key add - &&
echo "deb http://packages.elasticsearch.org/elasticsearch/1.3/debian stable main" > /tmp/elasticsearch.list &&
sudo cp /tmp/elasticsearch.list /etc/apt/sources.list.d/ &&
sudo apt-get update &&
sudo apt-get dist-upgrade &&
sudo apt-get install default-jdk elasticsearch g++ git libwthttp-dev libxml2-dev libboost-signals-dev libboost-locale-dev make  &&
sudo cp ./MeSHWeb/etc_initd_MeSHWeb.sh /etc/init.d/MeSHWeb.sh &&
echo "*  *    * * *   root    sh /etc/init.d/MeSHWeb.sh" >> /etc/crontab &&
ln -s /usr/share/Wt/resources ./MeSHWeb/resources &&
cd ./MeSHImport/ &&
git clone https://github.com/QHedgeTech/cpp-elasticsearch.git &&
make &&
cd ../MeSHWeb/ &&
ln -s ../MeSHImport/cpp-elasticsearch &&
make &&
cd ../ParseTreeStructure/ &&
ln -s ../MeSHImport/cpp-elasticsearch &&
make
