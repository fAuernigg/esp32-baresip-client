
#ifndef _SIPPHONE_H
#define _SIPPHONE_H

#include <string.h>
#include <PubSubClient.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define ENABLE_baresip 1

int sipPhoneInit();

void sipHandleCommand(PubSubClient* mqttClient, String mqtt_id, String msg);

#ifdef __cplusplus
}
#endif

#endif // _SIPPHONE_H
