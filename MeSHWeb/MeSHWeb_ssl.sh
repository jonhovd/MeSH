#!/bin/sh
./MeSHWeb --docroot . --config ./wt_config.xml --servername mesh.uia.no --https-listen 0.0.0.0:443 --ssl-certificate /etc/letsencrypt/live/mesh.uia.no/fullchain.pem --ssl-private-key /etc/letsencrypt/live/mesh.uia.no/privkey.pem --ssl-tmp-dh /etc/ssl/MeSH.pem
