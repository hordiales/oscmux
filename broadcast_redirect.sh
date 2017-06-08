#!/bin/bash

BROADCAST_NET=$1
PORT=$2

# sudo netstat -plnt | grep ':80'

# example: $./broadcast_redirect.sh 10.142.39.255 4330
#          $./broadcast_redirect.sh 5.0.0.255 4330
#          $./oscmux -i osc.udp://localhost:12345 -o osc.udp://192.168.8.255:4330
./oscmux -i osc.udp://localhost:$PORT -o osc.udp://$BROADCAST_NET:$PORT

