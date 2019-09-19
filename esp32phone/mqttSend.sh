#!/bin/bash

if [ $# -eq 0 ] ; then
	echo "Error params: specify baresip command in format: \"esp32phone_[MAC]\" cmd arg1 arg2"
	exit 1
fi

devicename=$1
shift
topic="baresip/command"
msg=""


cmd=$1
shift
params="$@"
token=42

msg="{\"command\": \"$cmd\", \"params\": \"$@\", \"token\": \"$token\"}"

x=$(mosquitto_pub --help | grep "help" | tail -n 1)
if [[ "x$x" == "x" ]] ; then
	echo "Install mosquitto-clients first."
	echo "apt-get install mosquitto-clients"
	exit 1	
fi

echo -e "\nSending message: \n'$msg'\n"
mqttserver=cspiel.at
mqttpass=jackson_oida
mqttport=1883
mqttuser=esp32phone
mqttpass=


mosquitto_pub -h $mqttserver -p $mqttport -t "$devicename/$topic"  -q 2 -i MyPcClient -u "esp32phone" -P "$mqttpass" -m "$msg"
