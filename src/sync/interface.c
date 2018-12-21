/**
 * User: zhangjian2
 */
#include "malloc.h"
#include <string.h>
#include "interface.h"

public void deleteSyncArgs(sync_args* args)
{
	if (args) {
		free(args);
	}
}

public void deleteSync(_sync* sc)
{
	if (sc) {
		deleteSyncArgs(sc->args);
		sc->args = NULL;
		free(sc);
	}
}

public _sync* newSync(
	byte type,
	byte flag,
	onStart start,
	onEnd end,
	onRun run,
	void* run_args,
	char* name)
{
	if (isValidType(type) < 0) {
		// debug
		return NULL;
	}

	_sync* sc = (_sync*)malloc(sizeof(_sync));
	if (!sc) {
		// debug
		return NULL;
	}

	sc->id = 0;
	sc->type = type;
	sc->flag = flag;
	sc->start = NOT_START;
	sc->args = (sync_args*)malloc(sizeof(sync_args));
	if (!sc->args) {
		// debug
		deleteSync(sc);
		return NULL;
	}

	sc->args->start = start;
	sc->args->end = end;
	sc->args->run = run;
	sc->args->run_args = run_args;
	strcpy(sc->args->name, name);
	return sc;
}

public byte isValidType(byte type)
{
	switch (type) {
		case THREAD:
		case PROCESS:
			return S_OK;
		default:
			return S_ERR;
	}
}

public void join(_sync* sc)
{
	CALL(sc, Join);
}

public void start(_sync* sc)
{
	CALL(sc, Start);
}

public void detach(_sync* sc)
{
	CALL(sc, Detach);
}

public void callSync(_sync* sc)
{
	if (!sc || !sc->start) {
		return;
	}

	if (sc->args->start) {
		sc->args->start(sc->id, sc->args->name);
	}

	void* result = NULL;
	if (sc->args->run) {
		result = sc->args->run(sc->id, sc->args->name, sc->args->run_args);
	}

	if (sc->args->end) {
		sc->args->end(sc->id, sc->args->name, result);
	}

	if (result) {
		free(result);
	}
}