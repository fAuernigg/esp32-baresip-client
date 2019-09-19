#!/bin/bash

devicename=esp32phone_CC:50:E3:9C:53:CC
topic="baresip/command"
msg=""

if [ $# -eq 0 ] ; then
	echo "Error params: specify baresip command in format: cmd arg1 arg2"
	exit 1
fi

cmd=$1
shift
params="$@"
token=42

msg="{\"command\": \"$1\", \"params\": \"$@\", \"token\": \"$token\"}"

x=$(mosquitto_pub --help | grep "help" | tail -n 1)
if [[ "x$x" == "x" ]] ; then
	echo "Install mosquitto-clients first."
	echo "apt-get install mosquitto-clients"
	exit 1	
fi

echo "sending message: '$1'"
mqttserver=cspiel.at
mqttpass=jackson_oida
mqttport=1883
mqttuser=esp32phone
mqttpass=

mosquitto_pub -h $mqttserver -p $mqttport -t "$devicename/$topic"  -q 2 -i MyPcClient -u "esp32phone" -P "$mqttpass" -m "$msg"
