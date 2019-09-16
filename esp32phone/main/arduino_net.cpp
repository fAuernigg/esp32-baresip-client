
#include "arduino_net.h"
#include <WiFi.h>

const char *ard_local_ip()
{
	return WiFi.localIP().toString().c_str();
}

const char *ard_gateway()
{
	return WiFi.gatewayIP().toString().c_str();
}
