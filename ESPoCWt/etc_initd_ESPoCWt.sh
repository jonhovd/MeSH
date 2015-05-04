#!/bin/sh

### BEGIN INIT INFO
# Provides:          ESPoCWt
# Required-Start:    $elasticsearch
# Required-Stop:     
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Starts ESPoCWt
# Description:       Starts ESPoCWt using start-stop-daemon
### END INIT INFO

cd /home/frg/Helsebib/ESPoCWt/
./ESPoCWt.sh 80 &
