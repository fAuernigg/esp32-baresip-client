#!/bin/bash

### This is a template script for making, deploying and upgrading.
### Copy this template and edit following variables.

mqttserver=xxx.org
mqttpass=xxx
mqttport=1883
mac=30:AE:A4:08:6F:34
deployserver=yyy
httpserver=yyy.org

./inc_buildnr.sh
v=$(cat BUILDNR)
touch main/main.cpp
echo "Make commando"
echo "make MQTTSERVER=${mqttserver} MQTTPORT=${mqttport} MQTTUSER=esp32phone MQTTPASS=${mqttpass} NOTLS=1 BUILDNR=${v}"

if make MQTTSERVER=${mqttserver} MQTTPORT=${mqttport} MQTTUSER=esp32phone MQTTPASS=${mqttpass} NOTLS=1 BUILDNR=${v}
then
	echo "Deploying"
	if scp build/app-template.bin ${deployserver}:/var/www/esp32phone
	then
		echo "Initiate Upgrade"
		mosquitto_pub -h ${mqttserver} -t esp32phone_${mac}/update -u esp32phone -P ${mqttpass} -m "http://${httpserver}/esp32phone/app-template.bin"

		make monitor
	fi
fi

