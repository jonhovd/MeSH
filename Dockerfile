# Use a lightweight base image
FROM debian:bullseye

# Oppdater og installer nÃ¸dvendige pakker som bash og andre avhengigheter
RUN apt-get update && apt-get install -y \
    gnupg1 \
    gnupg2 \
    wget \
    git \
    sudo \
    systemd \
    g++ \
    build-essential \
    libssl-dev \
    libxml2-dev \
    libboost-all-dev \
    cmake && \
    # Clean up to reduce image size
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*


RUN git clone https://github.com/sigrunespelien/MeSH.git && \
    cd MeSH 

# Update and install necessary packages, including Elasticsearch
RUN apt-get update && \
    apt-get install -y --no-install-recommends wget gnupg1 gnupg2 apt-transport-https && \
    wget -qO - https://artifacts.elastic.co/GPG-KEY-elasticsearch | apt-key add - && \
    echo "deb https://artifacts.elastic.co/packages/7.x/apt stable main" | tee /etc/apt/sources.list.d/elastic-7.x.list && \
    apt-get update && \
    apt-get install -y elasticsearch && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*



RUN sudo apt-get -y install g++ libssl-dev libxml2-dev libboost-all-dev cmake make && \
mkdir -p ~/projects/ && \
cd ~/projects/ && \
wget https://github.com/emweb/wt/archive/4.8.2.tar.gz && \
gunzip 4.8.2.tar.gz && \
tar xf 4.8.2.tar && \
rm 4.8.2.tar && \
rm -f wt && \
ln -s wt-4.8.2 wt && \
cd wt && \
mkdir build && \
cd build && \
cmake ../ -DENABLE_LIBWTDBO:BOOL=OFF && \
make -j2 && \
sudo make install && \
sudo ldconfig

RUN cd MeSH && \
ln -sf /usr/local/share/Wt/resources ./MeSHWeb/ && \
cd ./MeSHImport/ && \
git clone https://github.com/frodegill/cpp-elasticsearch.git && \
cd ../MeSHWeb/ && \
ln -sf ../MeSHImport/cpp-elasticsearch && \
cd .. && \
sudo mkdir -p /opt/Helsebib/MeSHWeb/ && \
sudo ln -sf /usr/local/share/Wt/resources /opt/Helsebib/MeSHWeb/

RUN cd MeSH && \
git pull && \
cd ./MeSHImport/cpp-elasticsearch/ && \
git pull && \
cd .. && \
make clean && \
make -j2 && \
cd ../MeSHWeb/ && \
make clean && \
make -j2 && \
sudo make install

# Expose the port that the web service will use
EXPOSE 8080
