#ifndef __LIBEVENT_HEADER
#define __LIBEVENT_HEADER

#include <event2/event.h>
#include "base.h"
#include "e.h"

public struct event_base* createLibBase(int);
public byte libAdd(struct event_base*, int, clientE*);
public void libLoop(struct event_base*, byte);
public void freeLibFreeEvent(struct event*);

#endif