#ifndef __RSTACK_H
#define __RSTACK_H

#include "base.h"

typedef struct rstack {
	void** n;
	int32_t size;
	int32_t top;
} rstack;

public rstack* newRStack(size_t, int32_t);

public boolean isEmptyRStack(rstack*);

public boolean pushRStack(rstack*, void*);

public void* popRStack(rstack*);

public void freeRStack(rstack*);

#endif