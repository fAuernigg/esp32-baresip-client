#define TAG "sipphone"


#include "sipphone.h"

#ifdef ENABLE_baresip

#ifndef UA_DISPLAY_NAME
#define UA_DISPLAY_NAME "FedEx esp32"
#endif

#ifndef UA_USER
#define UA_USER "esp32"
#endif

typedef uint32_t u32_t;
#include "cJSON.h"
#include "esp_log.h"
#include "version.h"

//#include <net/if.h>
#include <re.h>
#include <baresip.h>

#include "arduino_net.h"


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

extern void setCallState(bool value);

static void ua_event_handler(struct ua *ua, enum ua_event ev,
			     struct call *call, const char *prm, void *arg)
{
	switch (ev) {
		case UA_EVENT_CALL_INCOMING:
		case UA_EVENT_CALL_RINGING:
		case UA_EVENT_CALL_PROGRESS:
		case UA_EVENT_CALL_ESTABLISHED:
			setCallState(1);
			break;
		case UA_EVENT_CALL_CLOSED:
			setCallState(0);
			break;
		default:
			break;
	}
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
	conf_set(conf, "sip_listen", "0.0.0.0:5060");
	conf_set(conf, "rtp_stats", "no");
	conf_set(conf, "rtcp_enable", "no");
	conf_set(conf, "module", "g711");
	conf_set(conf, "module", "i2s\n");
	conf_set(conf, "module_app", "menu\n");

	conf_set(conf, "audio_player", "i2s\n");
	conf_set(conf, "audio_source", "i2s\n");
	conf_set(conf, "audio_alert", "i2s\n");
	conf_set(conf, "audio_channels", "1\n");
	conf_set(conf, "audio_srate", "8000\n");

	return config_parse_conf(conf_config(), conf);
}


#define STACK_SIZE 16*1024

int sipPhoneInit()
{
	char versionBuffer[32];
	bool udp = true, tcp = false, tls = false;
	int err = 0;
	struct mbuf *mb;
	String ip = ard_local_ip().toString();

	ESP_LOGI(TAG, "Starting Baresip");

	err = libre_init();
	if (err) {
		ESP_LOGE(TAG, "Could not initialize libre");
		goto baresip_error;
	}


    ESP_LOGI(TAG, "Baresip logging activated");
    log_register_handler(&baresipLog);
    log_enable_debug(true);

	err = conf_configure();
	err |= extern_baresip_config(conf_cur());
	if (err) {
		ESP_LOGI(TAG, "Could not configure.");
		return err;
	}

	err = baresip_init(conf_config());
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

	err = ua_init(versionBuffer, udp, tcp, tls);
	if (err) {
		ESP_LOGE(TAG, "Could not initialize baresip user agent");
		goto baresip_error;
	}

	uag_set_exit_handler(ua_exit_handler, NULL);

	err = conf_modules();
	if (err) {
		ESP_LOGE(TAG, "Could not configure modules");
		goto baresip_error;
	}

	uag_event_register(ua_event_handler, NULL);

	mb = mbuf_alloc(100);
	mbuf_printf(mb, "\"" UA_DISPLAY_NAME "\" "
			"<sip:%s@%s>;auth_pass=none;regint=0;answermode=auto", UA_USER, ip.c_str());
	mbuf_write_u8(mb, 0);
	mbuf_set_pos(mb, 0);
	ua_alloc(NULL, (char *) mbuf_buf(mb));
	mem_deref(mb);

	info("%s stack size = %ld", __func__, STACK_SIZE);
	xTaskCreate(baresip_main, "baresipmain", STACK_SIZE, NULL, 10, &baresip_thread);

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


char* gai_strerror(int e) {
	return (char*) "";
}

// TODO: use correct function instead of dummy impl
int net_rt_list(net_rt_h *rth, void *arg) {
	struct sa dst;
	struct sa gw;

	if (!rth)
		return EINVAL;

	sa_init(&dst, AF_INET);

	String ip = ard_gateway().toString();
	sa_set_str(&gw, ip.c_str(), 0);

	rth("wifi", &dst, 24, &gw, arg);
	return 0;
}

int net_if_getaddr4(const char *ifname, int af, struct sa *ip)
{
	int err;
	String s = ard_local_ip().toString();
	ESP_LOGI(TAG, "ard_local_ip = %s", s.c_str());
	err = sa_set_str(ip, s.c_str(), 0);
	if (err)
		ESP_LOGW(TAG, "%s No valid local ip address\n", __FUNCTION__);

	return err;
}

int net_if_list(net_ifaddr_h *ifh, void *arg)
{
	struct sa sa;
	int err;
	if (!ifh)
		return EINVAL;

	String ip = ard_local_ip().toString();
	ESP_LOGI(TAG, "ard_local_ip = %s", ip.c_str());
	err = sa_set_str(&sa, ip.c_str(), 0);
	if (err) {
		ESP_LOGW(TAG, "%s No valid local ip address\n", __FUNCTION__);
		return err;
	}
	ifh("wifi", &sa, arg);

	return 0;
}

int net_if_getaddr_for(const struct pl *dest, struct sa *localip, bool isip)
{
	(void) dest;
	int err;

	if (!localip)
		return EINVAL;

	if (!isip) {
		ESP_LOGW(TAG, "Only IP-addresses are supported for the peer address\n");
		return EINVAL;
	}

	String ip = ard_local_ip().toString();
	err = sa_set_str(localip, ip.c_str(), 0);
	if (err)
		ESP_LOGW(TAG, "%s No valid local ip address\n", __FUNCTION__);

	return err;
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
	return 0;
}

void *_mod_sym(void *h, const char *symbol) {
	ESP_LOGI(TAG, "%s: %d", __FUNCTION__, __LINE__);
	return 0;
}
void  _mod_close(void *h) {
}


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

	if (!oe_cmd || !oe_prm || !oe_tok
		|| oe_cmd.length()==0) {
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

#ifdef __cplusplus
}
#endif


#else //ENABLE_baresip


#ifdef __cplusplus
extern "C" {
#endif

int sipPhoneInit() {
	return true;
}

void sipHandleCommand(PubSubClient* mqttClient, String mqtt_id, String msg) {

}


#ifdef __cplusplus
}
#endif

#endif
