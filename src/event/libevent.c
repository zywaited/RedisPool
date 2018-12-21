#include "libevent.h"
#include "debug/debug.h"

public struct event_base* createLibBase(int maxSize)
{
	((void*)&maxSize);
	return event_base_new();
}

public byte libAdd(
	struct event_base* base,
	int fd,
	clientE* ce)
{
	if (!ce) {
		return S_OK;
	}
	
	ce->e = event_new(base, fd, ce->flag, ce->func, ce->args);
	if (!ce->e) {
		debug(DEBUG_ERROR, "Can't create libevent");
		return S_ERR;
	}

	if (event_add(ce->e, ce->time) < 0) {
		debug(DEBUG_ERROR, "Can't add libevent");
		return S_ERR;
	}

	return S_OK;
}

public void libLoop(struct event_base* base, byte flag)
{
	event_base_loop(base, flag);
}

public void freeLibFreeEvent(struct event* e)
{
	if (!e) {
		return;
	}
	
	if (0 > event_del(e)) {
		debug(DEBUG_ERROR, "Can't free libevent");
	}

	event_free(e);
}