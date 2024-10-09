# Base image
FROM debian:bullseye

# Update and install essential packages
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
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Clone the MeSH repository
RUN git clone https://github.com/sigrunespelien/MeSH.git && \
    cd MeSH

# Install Elasticsearch and its dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    wget \
    gnupg1 \
    gnupg2 \
    apt-transport-https && \
    wget -qO - https://artifacts.elastic.co/GPG-KEY-elasticsearch | apt-key add - && \
    echo "deb https://artifacts.elastic.co/packages/7.x/apt stable main" | tee /etc/apt/sources.list.d/elastic-7.x.list && \
    apt-get update && \
    apt-get install -y elasticsearch && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Install Wt library and setup projects directory
RUN apt-get update && \
    apt-get install -y g++ libssl-dev libxml2-dev libboost-all-dev cmake make && \
    mkdir -p ~/projects/ && \
    cd ~/projects/ && \
    wget https://github.com/emweb/wt/archive/4.8.2.tar.gz && \
    tar -xzf 4.8.2.tar.gz && \
    rm 4.8.2.tar.gz && \
    ln -s wt-4.8.2 wt && \
    cd wt && \
    mkdir build && \
    cd build && \
    cmake ../ -DENABLE_LIBWTDBO:BOOL=OFF && \
    make -j2 && \
    sudo make install && \
    sudo ldconfig

# Link resources for the MeSH project
RUN cd MeSH && \
    ln -sf /usr/local/share/Wt/resources ./MeSHWeb/ && \
    cd ./MeSHImport/ && \
    git clone https://github.com/frodegill/cpp-elasticsearch.git && \
    cd ../MeSHWeb/ && \
    ln -sf ../MeSHImport/cpp-elasticsearch && \
    cd .. && \
    sudo mkdir -p /opt/Helsebib/MeSHWeb/ && \
    sudo ln -sf /usr/local/share/Wt/resources /opt/Helsebib/MeSHWeb/

# Build the MeSH project
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

# Install additional tools
RUN apt-get update && apt-get install -y \
    bash \
    curl \
    wget \
    vim \
    software-properties-common && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Set up the application directory
WORKDIR /usr/src/app
COPY . .

# Set permissions for Bash scripts
RUN chmod +x /usr/src/app/src/*.sh

# Expose the port used by the web service
EXPOSE 8080

# Run the install script on startup
CMD ["bash", "/usr/src/app/src/install_wt.sh"]



# Bygg Docker image:
# docker build -t my-mesh-project .

# Kjør containeren:
# docker run -it my-mesh-project

#Kjørcontaineren:
# docker run -it -p 8080:8080 --name mesh my-mesh-project /bin/bash