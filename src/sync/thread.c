/**
 * User: zhangjian2
 */

#include "thread.h"
#include "debug/debug.h"

public void threadJoin(_sync* sc)
{
	if (!sc ||
		sc->type != THREAD ||
		!sc->id ||
		!sc->start) {
		return;
	}

	int status = pthread_join((pthread_t)sc->id, NULL);
	if (status != 0) {
		debug(DEBUG_WARNING, "Can't join the thread[%d]", (int)sc->id);
	} else {
		debug(DEBUG_NOTICE, "Join the thread[%d]", (int)sc->id);
	}
}

public void threadStart(_sync* sc)
{
	if (!sc || sc->type != THREAD || sc->start) {
		return;
	}
	
	if (pthread_create((pthread_t*)&sc->id, NULL, threadDeal, sc) != 0) {
		// debug
		debug(DEBUG_ERROR, "[%d]Can't create sub thread", pthread_self());
		return;
	}
}

public void threadDetach(_sync* sc)
{
	if (!sc || sc->type != THREAD || sc->start) {
		return;
	}
	
	int status = pthread_detach((pthread_t)sc->id);
	if (status != 0) {
		// debug
		debug(DEBUG_ERROR, "Can't detach thread[%d]", (int)sc->id);
	}
}

public void* threadDeal(void* arg)
{
	_sync* sc = (_sync*)arg;
	if (!sc ||
		sc->type != THREAD ||
		!sc->args ||
		sc->start
	) {
		return NULL;
	}

	if (sc->flag) {
		threadDetach(sc);
	}

	sc->start = THREAD_START;
	callSync(sc);
	return NULL;
}