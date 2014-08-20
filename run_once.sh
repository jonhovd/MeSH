#!/bin/sh
echo "deb http://packages.elasticsearch.org/elasticsearch/1.3/debian stable main" > /tmp/elasticsearch.list &&
sudo cp /tmp/elasticsearch.list /etc/apt/sources.list.d/ &&
sudo apt-get update &&
sudo apt-get dist-upgrade &&
sudo apt-get install default-jdk elasticsearch g++ git libwthttp-dev libxml2-dev make  &&
ln -s /usr/share/Wt/resources ESPoCWt/resources &&
cd ESPoCImport &&
git clone https://github.com/QHedgeTech/cpp-elasticsearch.git &&
make &&
cd ../ESPoCWt &&
ln -s ../ESPoCImport/cpp-elasticsearch &&
make
