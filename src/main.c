#include <stdio.h>
#include "config.h"
#include "dnc.h"
#include "thread.h"
#include "log.h"
#include "dnctty.h"
#include "confuse.h"
#include "dnc_thread.h"
#include <string.h>

cfg_t *cfg;
cfg_opt_t port_opts[] = {
    CFG_STR("device", 0, CFGF_NONE),
    CFG_STR("type", 0, CFGF_NONE),
    CFG_STR("gcode", 0, CFGF_NONE),
    CFG_INT("speed", 0, CFGF_NONE),
    CFG_END()
};

cfg_opt_t opts[] = {
	CFG_SEC("port", port_opts, CFGF_MULTI | CFGF_TITLE),
	CFG_FUNC("include", &cfg_include),
	CFG_END()
};

struct port_thread_list_t {
	struct dnc_context context;
	struct Thread * thread;
	struct port_thread_list_t * next;
};

struct port_thread_list_t * port_thread_list = NULL;
struct port_thread_list_t * port_thread_list_tail = NULL;

int main (int argc, char **argv) {
	cfg = cfg_init(opts, CFGF_NOCASE);
    int ret = cfg_parse(cfg, argc > 1 ? argv[1] : "test.conf");
	// TODO - validate port function

	int n = cfg_size(cfg, "port");
    dnc_log(LOG_DEBUG,"%d configured ports:", n);
    for (int i = 0; i < n; i++) {
        cfg_t *pxy;
        cfg_t *bm = cfg_getnsec(cfg, "port", i);

		dnc_log(LOG_DEBUG, "  port #%u (%s):", i + 1, cfg_title(bm));
        dnc_log(LOG_DEBUG, "    device = %s", cfg_getstr(bm, "device"));
        dnc_log(LOG_DEBUG, "    speed = %d", (int)cfg_getint(bm, "speed"));
        dnc_log(LOG_DEBUG, "    type = %s", cfg_getstr(bm, "type"));
        dnc_log(LOG_DEBUG, "    gcode = %s", cfg_getstr(bm, "gcode"));
		struct port_thread_list_t * tle = calloc(1, sizeof(struct port_thread_list_t));
		tle->context.port = cfg_getstr(bm, "device");
		tle->context.gcode_file = cfg_getstr(bm, "gcode");
		int speed = (int)cfg_getint(bm,"speed");
		if (speed == 9600) {
			tle->context.speed = B9600;
		} else if (speed == 4800) {
			tle->context.speed = B4800;
		} else if (speed == 19200) {
			tle->context.speed = B19200;
		} else if (speed == 115200) {
			tle->context.speed = B115200;
		} else {
			dnc_log(LOG_ERR, "Invalid speed %i", speed);
			exit(1);
		}
		char * type = cfg_getstr(bm, "type");
		if (type == NULL) {
			dnc_log(LOG_ERR, "Type is missing");
			exit(1);
		}
		if (strcmp(type, "haas") == 0)
			tle->context.dnc_type = DNC_TYPE_HAAS;
		else if (strcmp(type, "mori") == 0) {
			tle->context.dnc_type = DNC_TYPE_MORI;
		} else {
			dnc_log(LOG_ERR, "DNC Type invalid: %s", type);
			exit(1);
		}

		
		if (port_thread_list_tail == NULL) {
			port_thread_list_tail = tle;
			port_thread_list = tle;
		} else {
			port_thread_list_tail->next = tle;
			port_thread_list_tail = tle;
		}
	} 

    if (port_thread_list == NULL) {
        dnc_log(LOG_ERR, "No ports configured");
        exit(1);
    }

    struct port_thread_list_t * pte = port_thread_list;
    while (pte != NULL) {
        pte->thread = Thread_Launch(&dnc_thread, NULL, "port", &pte->context);
        pte = pte->next;
    }

    bool running = true;
    while (Thread_getCount() != 0) {
        sleep(1000);
    }
    return 0;
}



