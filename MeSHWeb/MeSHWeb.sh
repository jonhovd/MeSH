#!/bin/sh
./MeSHWeb --docroot . --config ./wt_config.xml --http-listen 0.0.0.0:$1
