/**
 * Grok libevent
 *
 * https://getpocket.com/a/read/791931038
 * http://www.wangafu.net/~nickm/libevent-book/Ref3_eventloop.html
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <event2/event.h>
#include <string.h>

const int OK = 0;
const int NOT_OK = 1;

struct event_config *cfg;
struct event_base *base;
struct event *hup_event;

void log_debug(char *txt) {
	printf("%s\n", txt);
}

void on_sighup(evutil_socket_t fd, short what, void *arg)
{
    //struct event *me = arg;
    log_debug("on_sighup enter");
}


int init() {
	int i;
	// My program wants to use edge-triggered events if at all possible.
	// So I'll try to get a base twice: Once insisting on edge-triggered IO, and once not.
	for (i = 0; i < 2; ++i) {
		cfg = event_config_new();
		/* I don't like select. */
		event_config_avoid_method(cfg, "select");
		if (i == 0) {
			event_config_require_features(cfg, EV_FEATURE_ET);
		}
		base = event_base_new_with_config(cfg);
		event_config_free(cfg);
		if (base) {
			return OK;
		}
	}
	return NOT_OK;
}


int check() {
	int i;
	enum event_method_feature f;
	const char **methods = event_get_supported_methods();
	printf("libevent version: %s.  Available methods are:\n", event_get_version());
	for (i = 0; methods[i] != NULL; ++i) {
		printf("    %s\n", methods[i]);
	}
	if (!base) {
		return NOT_OK;
	}
	printf("Using libevent with backend method: %s.\n", event_base_get_method(base));
	f = event_base_get_features(base);
	if ((f & EV_FEATURE_ET)) {
		log_debug("  Edge-triggered events are supported.");
	}
	if ((f & EV_FEATURE_O1)) {
		log_debug("  O(1) event notification is supported.");
	}
	if ((f & EV_FEATURE_FDS)) {
		log_debug("  All FD types are supported.");
	}
	log_debug("");
	return OK;
}

void deinit() {
	if (base) {
		event_base_free(base);
	}
	libevent_global_shutdown();
}

///* Here's a callback function that calls loopbreak */
//void cb(int sock, short what, void *arg) {
//	struct event_base *base = arg;
//	event_base_loopbreak(base);
//}

static int n_calls = 0;

void on_timer(evutil_socket_t fd, short what, void *arg)
{
    struct event *me = arg;
    printf("cb_func called %d times so far.\n", ++n_calls);
    if (n_calls >= 10)
       event_del(me);
}

//void main_loop(struct event_base *base, evutil_socket_t watchdog_fd) {
//	struct event *watchdog_event;
//	log_debug("main loop");
//	/* Construct a new event to trigger whenever there are any bytes to
//	 read from a watchdog socket.  When that happens, we'll call the
//	 cb function, which will make the loop exit immediately without
//	 running any other active events at all.
//	 */
//	watchdog_event = event_new(base, watchdog_fd, EV_READ, cb, base);
//
//	event_add(watchdog_event, NULL);
//
//	event_base_dispatch(base);
//}

void periodic_work() {
	struct timeval period = { 1, 0 };
//	period.tv_sec = 1;
//	period.tv_usec = 0;

//	while (1) {
//		event_base_loopexit(base, &period);
//		// event_base_dump_events(base, stdout);
//		event_base_dispatch(base);
//		log_debug("work");
//	}

	struct event *ev;
	log_debug("work enter");
	// setup a repeating timer to get called called N times
	ev = event_new(base, -1, EV_PERSIST | EV_READ, on_timer, event_self_cbarg());
	event_add(ev, &period);

	// call sighup_function on a HUP signal
	//hup_event = evsignal_new(base, SIGHUP, on_sighup, NULL);
//	struct event *ev2;
//	ev2 = event_new(base, SIGHUP, EV_SIGNAL|EV_PERSIST, on_sighup, NULL);
//	event_add(ev2, NULL);

	event_base_dispatch(base);

	log_debug("work exit");
}

int main1(void) {
	int rc;
	rc = init();
	if (rc == OK) {
		check();
		periodic_work();
	}
	deinit();
	return EXIT_SUCCESS;
}
