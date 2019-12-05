#ifndef ARDUINO_NET_H
#define ARDUINO_NET_H

#include <stdio.h>

void ard_get_network_address(char *local_ip, size_t maxLen,
							 char *gw, size_t maxLenGw);

#endif
