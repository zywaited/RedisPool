#include "map.h"
#include "debug/debug.h"
#include <string.h>

public mapValue* newMapValue(void* value, boolean isMalloc)
{
	mapValue* mv = (mapValue*)malloc(sizeof(mapValue));
	if (!mv) {
		return NULL;
	}

	mv->value = value;
	mv->isMalloc = isMalloc;
	return mv;
}

public mapKey* newMapKey(uint64_t h, const char* str)
{
	mapKey* key = (mapKey*)malloc(sizeof(mapKey));
	if (!key) {
		return NULL;
	}

	size_t strKeyLen = 0;
	if (str) {
		strKeyLen = strlen(str);
	}

	if (strKeyLen > 0) {
		char* tmp = (char*)malloc(sizeof(char) * (strKeyLen + 1));
		if (!tmp) {
			freeMapKey(key);
			return NULL;
		}

		strncpy(tmp, str, strKeyLen);
		tmp[strKeyLen] = '\0';
		key->str = tmp;
	} else {
		key->str = NULL;
	}

	key->h = h;
	return key;
}

public void freeMapValue(mapValue* mv, freeFunc func)
{
	if (!mv) {
		return;
	}

	if (mv->isMalloc && mv->value) {
		if (func) {
			func(mv->value);
		} else {
			free(mv->value);
		}

		mv->value = NULL;
	}

	free(mv);
}

public uint64_t hashCode(const char* str)
{
 	size_t len = strlen(str);
 	uint64_t h = 0;
 	if (len <= 0) {
 		return h;
 	}

 	size_t index = 0;
 	for (; index < len; index++) {
 		h = h * 31 + *str;
 		str++;
 	}

 	return h;
}

public void freeMapKey(mapKey* key)
{
	if (!key) {
		return;
	}

	if (key->str) {
		free(key->str);
		key->str = NULL;
	}

	free(key);
}

public symbol* newSymbol(char* funcName, freeFunc func)
{
	size_t len = 0;
	if (funcName) {
		len = strlen(funcName);
	}

	if (len > freeNameLen || len <= 0) {
		return NULL;
	}

	symbol* sl = (symbol*)malloc(sizeof(symbol));
	if (!sl) {
		return NULL;
	}

	memset(sl->name, 0, freeNameLen);
	strncpy(sl->name, funcName, len);
	sl->func = func;
	return sl;
}

public void freeSymbol(void* sl)
{
	if (!sl) {
		return;
	}

	free(sl);
}