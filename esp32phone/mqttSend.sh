x=$(mosquitto_pub --help | grep "help" | tail -n 1)
if [[ "x$x" == "x" ]] ; then
	echo "Install mosquitto-clients first."
	echo "apt-get install mosquitto-clients"
	exit 1	
fi

echo "sending message: '$1'"
mosquitto_pub -h mrxa.ga -p 8883 -t "esp32phone_24:0A:C4:03:A3:0C/cmd"  -q 2 --capath "/etc/ssl/certs" -i MyPcClient -u "mkabuild" -P "3A3ks34sddfksdfolsL3jhjsh__a}3{m" -m "$1"
