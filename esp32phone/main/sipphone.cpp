#define TAG "sipphone"


#include "sipphone.h"

#ifdef ENABLE_baresip

typedef uint32_t u32_t;
#include "cJSON.h"
#include "esp_log.h"
#include "version.h"

//#include <net/if.h>
#include <re.h>
#include <baresip.h>


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

	// NOTE: modules must be unloaded after all application activity has stopped.
	ESP_LOGW(TAG, "main: unloading modules..\n");
	mod_close();

	log_unregister_handler(&baresipLog);

	libre_close();

	// Check for memory leaks
	tmr_debug();
	mem_debug();

	ESP_LOGI(TAG, "Exiting baresip thread");

	return;
}


int extern_baresip_config(struct conf *conf)
{
	conf_set(conf, "module", "g711");
	conf_set(conf, "module", "aui2s\n");
	conf_set(conf, "module_app", "stdio\n");
	conf_set(conf, "module_app", "menu\n");

	conf_set(conf, "audio_player", "aui2s\n");
	conf_set(conf, "audio_source", "aui2s\n");
	conf_set(conf, "audio_alert", "aui2s\n");
	conf_set(conf, "audio_channels", "1\n");
	conf_set(conf, "audio_srate", "8000\n");

	return 0;
}


int sipPhoneInit()
{
	char versionBuffer[32];
	bool udp = true, tcp = true, tls = true, prefer_ipv6 = false;
	int err = 0;

	ESP_LOGI(TAG, "Starting Baresip");

	err = libre_init();
	if (err) {
		ESP_LOGE(TAG, "Could not initialize libre");
		goto baresip_error;
	}


    ESP_LOGI(TAG, "Baresip logging activated");
    log_register_handler(&baresipLog);
    log_enable_debug(true);

	err = baresip_init(conf_config(), false);
	ESP_LOGI(TAG, "Baresip %s: %d", __FUNCTION__, __LINE__);

	if (err) {
		ESP_LOGE(TAG, "Could not initialize baresip");
		goto baresip_error;
	}
	ESP_LOGI(TAG, "Baresip %s: %d", __FUNCTION__, __LINE__);

	 if (str_isset(conf_config()->audio.audio_path)) {
		play_set_path(baresip_player(),
				  conf_config()->audio.audio_path);
	}

	 strcpy(versionBuffer, "baresip esp32 ");
	 strcat(versionBuffer, VERSION);

	ESP_LOGI(TAG, "Using this as baresip version string: %s", versionBuffer);

	err = ua_init(versionBuffer, udp, tcp, tls, prefer_ipv6);
	if (err) {
		ESP_LOGE(TAG, "Could not initialize baresip user agent");
		goto baresip_error;
	}

	uag_set_exit_handler(ua_exit_handler, NULL);
	uag_event_register(ua_event_handler, NULL);

	xTaskCreate(baresip_main, "baresipmain", 2048, NULL, 10, &baresip_thread);

	ESP_LOGI(TAG, "Baresip initialization done");

	return true;

baresip_error:
  ua_stop_all(true);

  ua_close();
  conf_close();

  baresip_close();

  // NOTE: modules must be unloaded after all application activity has stopped.
  ESP_LOGW(TAG, "main: unloading modules..\n");
  mod_close();

  log_unregister_handler(&baresipLog);

  libre_close();

  // Check for memory leaks
  tmr_debug();
  mem_debug();

  return false;
}

#ifdef __cplusplus
extern "C" {
#endif

// TODO: use correct function instead of dummy impl
int net_rt_list(net_rt_h *rth, void *arg) {
	return 0;
}


#include <signal.h>
#include <pwd.h>

sighandler_t signal(int signum, sighandler_t handler)
{
	return (sighandler_t) 0;
}

char *  getlogin(void) {
	return (char*) "";
}


struct passwd	*getpwnam (const char *) {
	return 0;
}

// libre apis to load modules (which we use statically)
void *_mod_open(const char *name) {
	ESP_LOGI(TAG, "%s: %d", __FUNCTION__, __LINE__);
	return 0;
}

void *_mod_sym(void *h, const char *symbol) {
	ESP_LOGI(TAG, "%s: %d", __FUNCTION__, __LINE__);
	return 0;
}
void  _mod_close(void *h) {
	ESP_LOGI(TAG, "%s: %d", __FUNCTION__, __LINE__);
}



const struct mod_export *mod_table[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	// TODO provide functions in table
	// &exports_wincons,
	// &exports_g711,
	// &exports_winwave,
	// &exports_dshow,
	// &exports_account,
	// &exports_contact,
	// &exports_menu,
	// &exports_auloop,
	// &exports_vidloop,
	// &exports_uuid,
	// &exports_stun,
	// &exports_turn,
	// &exports_ice,
	// &exports_vumeter,
	NULL
};





#ifdef __cplusplus
}
#endif





static int mbuf_print_handler(const char *p, size_t size, void *arg)
{
	struct mbuf *mb = (struct mbuf*) arg;

	return mbuf_write_mem(mb, (const uint8_t*)p, size);
}



void sipHandleCommand(PubSubClient* mqttClient, String mqtt_id, String msg)
{
	cJSON *root = cJSON_Parse(msg.c_str());
	if (!root) {
		ESP_LOGE(TAG, "failed to decode baresip json received: %s", msg.c_str());
		return;
	}

	String oe_cmd, oe_prm, oe_tok;
	oe_cmd = String(cJSON_GetObjectItem(root,"command")->valuestring);
	oe_prm = String(cJSON_GetObjectItem(root,"params")->valuestring);
	oe_tok = String(cJSON_GetObjectItem(root,"token")->valuestring);

	if (oe_cmd.length()==0) {
		ESP_LOGE(TAG, "failed to decode baresip command: %s", msg.c_str());
		cJSON_Delete(root);
		return;
	}

	if (oe_prm.length()>0)
		oe_cmd += " " + oe_prm;
	ESP_LOGI(TAG, "handle baresip command: %s", oe_cmd.c_str());

	int err=0;
	struct mbuf *resp = mbuf_alloc(1024);
	struct re_printf pf = {mbuf_print_handler, resp};
	// Relay message to long commands
	err = cmd_process_long(baresip_commands(),
					oe_cmd.c_str(),
					oe_cmd.length(),
					&pf, NULL);
	if (err) {
		ESP_LOGE(TAG, "failed to process baresip command (cmd_process_long) (%d)\n", err);
	}

	String resp_topic = mqtt_id + "/baresip/command_resp/";
	if (oe_tok.length()>0)
		resp_topic += oe_tok;
	else
		resp_topic +=  "nil";

	String resp_msg;
	for (int i = 0; i < resp->end; i++)
		resp_msg += (char)(resp->buf[i]);

	if (!mqttClient->publish(resp_topic.c_str(), resp_msg.c_str())) {
		ESP_LOGE(TAG, "failed to publish baresip command response (%d)\n", err);
	}

	if (resp)
		mem_deref(resp);

	cJSON_Delete(root);
}


#else //ENABLE_baresip
int sipPhoneInit() {
	return true;
}

void sipHandleCommand(PubSubClient* mqttClient, String msg) {

}

#endif
