#include "rstack.h"

public rstack* newRStack(size_t per, int32_t size)
{
	if (size <= 0 || per == 0) {
		return NULL;
	}

	rstack* rsk = (rstack*)malloc(sizeof(rstack));
	if (!rsk) {
		return NULL;
	}

	rsk->n = (void**)malloc(per * size);
	if (!rsk->n) {
		freeRStack(rsk);
		return NULL;
	}

	rsk->size = size;
	rsk->top = -1;
	return rsk;
}

public boolean isEmptyRStack(rstack* rsk)
{
	return !rsk || rsk->top == -1 ? true : false;
}

public boolean pushRStack(rstack* rsk, void* n)
{
	// 满数量
	if (!rsk || rsk->top >= rsk->size - 1) {
		return false;
	}

	rsk->n[++rsk->top] = n;
	return true;
}

public void* popRStack(rstack* rsk)
{
	if (!rsk || rsk->top == -1) {
		return NULL;
	}

	return rsk->n[rsk->top--];
}

public void freeRStack(rstack* rsk)
{
	if (!rsk) {
		return;
	}

	if (rsk->n) {
		free(rsk->n);
		rsk->n = NULL;
	}

	free(rsk);
}