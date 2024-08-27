#!/bin/bash
# Please run before :
# apt-get update
# apt-get install
#Hvis koden mangler permission kjør kodene under
#chmod +x ./startMeSHWeb.sh
#chmod a+x ./startMeSHWeb.sh

# Install sudo
echo "Installing sudo..."
apt-get install -y sudo && echo "sudo installed successfully" || { echo "Failed to install sudo"; exit 1; }

# Install build-essential package
echo "Installing build-essential..."
sudo apt-get install -y build-essential && echo "build-essential installed successfully" || { echo "Failed to install build-essential"; exit 1; }

# Install systemd
echo "Installing systemd..."
apt-get install -y systemd && echo "systemd installed successfully" || { echo "Failed to install systemd"; exit 1; }

# Install wget
echo "Installing wget..."
sudo apt-get install -y wget && echo "wget installed successfully" || { echo "Failed to install wget"; exit 1; }

# Install gnupg1
echo "Installing gnupg1..."
sudo apt-get install -y gnupg1 && echo "gnupg1 installed successfully" || { echo "Failed to install gnupg1"; exit 1; }

# Install gnupg2
echo "Installing gnupg2..."
sudo apt-get install -y gnupg2 && echo "gnupg2 installed successfully" || { echo "Failed to install gnupg2"; exit 1; }

echo "Please reboot"
echo "..."
echo "..."
echo "Please reboot"
echo "..."
echo "..."
echo "Please reboot"
echo "..."
echo "..."
echo "Please reboot"
echo "..."
echo "..."
echo "Please reboot"


# "Run additional installation scripts"


#./install_elasticsearch.sh
#./install_wt.sh
#./mesh_runonce.sh
#./mesh_compile.sh
#sudo service elasticsearch start

#cd MeSHWeb
#./MeSHWeb --docroot . --config ./wt_config.xml --http-listen 0.0.0.0:8080 