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


case "$1" in
  start)
    cd /opt/Helsebib/MeSHWeb/
    while true
    do
      ./MeSHWeb.sh 80
    done
    ;;
  stop)
    pkill -u root MeSH
    ;;
  *)
    echo "Usage: /etc/init.d/MeSHWeb.sh {start|stop}"
    exit 2
    ;;
esac

exit 0
