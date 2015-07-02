#!/bin/sh

### BEGIN INIT INFO
# Provides:          MeSHWeb
# Required-Start:    $elasticsearch
# Required-Stop:     
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Starts MeSHWeb
# Description:       Starts MeSHWeb using start-stop-daemon
### END INIT INFO

cd /opt/Helsebib/MeSHWeb/
./MeSHWeb.sh 80 &
