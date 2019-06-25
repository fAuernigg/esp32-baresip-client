
#define TAG "sipphone"
#include "esp_log.h"
#include "sipphone.h"

#ifdef ENABLE_baresip

#include <re.h>
#include <baresip.h>
#include <string.h>

static TaskHandle_t baresip_thread;

static void baresipLogFunc(uint32_t level, const char *msg)
{
	(void)level;

    ESP_LOGI(TAG, "%s", msg);
}

static struct log baresipLog = {
	LE_INIT,
	baresipLogFunc
};


static void ua_exit_handler(void *arg)
{
	(void)arg;
	ESP_LOGW(TAG, "ua exited -- stopping main runloop\n");

	/* The main run-loop can be stopped now */
	re_cancel();
}

static void ua_event_handler(struct ua *ua, enum ua_event ev,
			     struct call *call, const char *prm, void *arg)
{
	re_printf("ua event: %s\n", uag_event_str(ev));
}


static void signal_handler(int sig)
{
	static bool term = false;

	if (term) {
		mod_close();
		exit(0);
	}

	term = true;

	ESP_LOGW(TAG, "terminated by signal %d\n", sig);

	ua_stop_all(false);
}

void baresip_main(void* arg)
{
	int err = 0;

	ESP_LOGI(TAG, "Entered baresip thread");

	err = re_main(signal_handler);

	if (err)
		ua_stop_all(true);

	ua_close();
	conf_close();

	baresip_close();

	/* NOTE: modules must be unloaded after all application
	 *       activity has stopped.
	 */
	ESP_LOGW(TAG, "main: unloading modules..\n");
	mod_close();

	log_unregister_handler(&baresipLog);

	libre_close();

	/* Check for memory leaks */
	tmr_debug();
	mem_debug();

	ESP_LOGI(TAG, "Exiting baresip thread");

	return;
}

bool sipPhoneInit() {
  const char *sipTransportLayerStr = "udp";
	const char *bfsipVersionStr = "0.1";
	char versionBuffer[256];
	bool udp = true, tcp = true, tls = true, prefer_ipv6 = false;
	int err = 0;
	FILE *pTempFile = NULL;

	ESP_LOGI(TAG, "Starting Baresip");

	err = libre_init();
	if (err) {
		ESP_LOGE(TAG, "Could not initialize libre");
		goto baresip_error;
	}


    ESP_LOGI(TAG, "Baresip logging activated");
    log_register_handler(&baresipLog);
    log_enable_debug(true);

	conf_path_set("/var/tmp/baresip");

	err = conf_configure();
	if (err) {
		ESP_LOGE(TAG, "Could not configure baresip");
		goto baresip_error;
	}

	err = baresip_init(conf_config(), false);

	if (err) {
		ESP_LOGE(TAG, "Could not initialize baresip");
		goto baresip_error;
	}

	 if (str_isset(conf_config()->audio.audio_path)) {
		play_set_path(baresip_player(),
				  conf_config()->audio.audio_path);
	}

	if (sipTransportLayerStr && !strcmp(sipTransportLayerStr, "TLS_ONLY")) {
		udp = false;
		tcp = false;
		ESP_LOGI(TAG, "Baresip listening to TLS only");
	}

	*versionBuffer = 0;
	strncat(versionBuffer, "commend SIP Series ", sizeof(versionBuffer));
	if (bfsipVersionStr) {
		char *pFound;

		strncat(versionBuffer, bfsipVersionStr, sizeof(versionBuffer));

		pFound = strchr(versionBuffer, '(');
		if (pFound)
			*(pFound - 1) = 0; // set space in front of '(' to zero
	} else {
		ESP_LOGW(TAG, "BF-SIP version undefined");
		strncat(versionBuffer, "X", sizeof(versionBuffer));
	}

	ESP_LOGI(TAG, "Using this as baresip version string: %s", versionBuffer);

	err = ua_init(versionBuffer, udp, tcp, tls, prefer_ipv6);
	if (err) {
		ESP_LOGE(TAG, "Could not initialize baresip user agent");
		goto baresip_error;
	}

	uag_set_exit_handler(ua_exit_handler, NULL);
	uag_event_register(ua_event_handler, NULL);

	err = conf_modules();
	if (err) {
		ESP_LOGE(TAG, "Could not configure baresip modules");
		goto baresip_error;
	}

	xTaskCreate(baresip_main, "baresipmain", 2048, NULL, 10, &baresip_thread);  

	ESP_LOGI(TAG, "Baresip initialization done");

	return true;

baresip_error:
  ua_stop_all(true);

  ua_close();
  conf_close();

  baresip_close();

  /* NOTE: modules must be unloaded after all application
    *       activity has stopped.
    */
  ESP_LOGW(TAG, "main: unloading modules..\n");
  mod_close();

  log_unregister_handler(&baresipLog);

  libre_close();

  /* Check for memory leaks */
  tmr_debug();
  mem_debug();

  return false;
}
#else
bool sipPhoneInit() {
	return true;
}
#endif