
#include "arduino_net.h"
#include <IPAddress.h>
#include <WiFi.h>


void ard_get_network_address(char *local_ip, size_t maxLen,
							 char *gw, size_t maxLenGw)
{
	strncpy(local_ip, WiFi.localIP().toString().c_str(), maxLen);
	strncpy(gw, WiFi.gatewayIP().toString().c_str(), maxLenGw);
}
