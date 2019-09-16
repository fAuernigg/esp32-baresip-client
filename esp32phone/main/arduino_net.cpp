
#include "arduino_net.h"
#include <WiFi.h>

IPAddress ard_local_ip()
{
	return WiFi.localIP();
}

IPAddress ard_gateway()
{
	return WiFi.gatewayIP();
}
