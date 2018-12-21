#ifndef __MAP_HEADER
#define __MAP_HEADER

#include "base.h"
#include <stdint.h>

typedef struct mapValue {
	void* value;
	boolean isMalloc;
} mapValue;

typedef struct mapKey {
	uint64_t h;
	char* str;
} mapKey;

typedef struct symbol {
	char name[freeNameLen];
	freeFunc func;
} symbol;

public mapValue* newMapValue(void*, boolean);

public mapKey* newMapKey(uint64_t, const char*);

public void freeMapValue(mapValue*, freeFunc);

public uint64_t hashCode(const char*);

public void freeMapKey(mapKey*);

public symbol* newSymbol(char*, freeFunc);
public void freeSymbol(void*);

#endif