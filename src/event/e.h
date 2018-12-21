#ifndef __E_H
#define __E_H

#include "base.h"
#include <event2/event.h>

typedef void (*eventCallback)(evutil_socket_t, short, void*);

typedef struct clientE {
	struct event* e;
	eventCallback func;
	void* args;
	short flag;
	struct timeval* time;
} clientE;

#endif