#!/bin/sh
./MeSHWeb --docroot . --config ./wt_config.xml --servername mesh.uia.no --http-listen 0.0.0.0:80 --https-listen 0.0.0.0:443 --ssl-certificate /etc/letsencrypt/live/mesh.uia.no/fullchain.pem --ssl-private-key /etc/letsencrypt/live/mesh.uia.no/privkey.pem --ssl-tmp-dh /etc/ssl/dh4096.pem
