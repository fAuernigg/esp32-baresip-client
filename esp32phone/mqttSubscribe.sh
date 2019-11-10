#!/bin/bash

if [ $# -eq 0 ] ; then
	topic="#"
else
	topic="$1"
fi

x=$(mosquitto_pub --help | grep "help" | tail -n 1)
if [[ "x$x" == "x" ]] ; then
	echo "Install mosquitto-clients first."
	echo "apt-get install mosquitto-clients"
	exit 1
fi

mqttserver=
mqttpass=
mqttport=1883
mqttuser=


mosquitto_sub -h $mqttserver -p $mqttport -t "$topic"  -q 2 -i MyPcClient -u "esp32phone" -P "$mqttpass" -v
