#include "types.h"
#include <string.h>
#include <ctype.h>
#include <limits.h>

public void freeGlobal(global* globals)
{
	if (!globals) {
		return;
	}

	if (globals->children) {
		freeTree(globals->children);
		globals->children = NULL;
	}

	free(globals);
}

public buffer* newBuff()
{
	buffer* buff = (buffer*)malloc(sizeof(buffer));
	if (!buff) {
		debug(DEBUG_ERROR, "Can't create buffer");
		return NULL;
	}

	// 额外申请一个字符
	buff->max = PER_IOBUF_SIZE + 1;
	buff->buf = (char*)malloc(sizeof(char) * buff->max);
	if (!buff->buf) {
		debug(DEBUG_ERROR, "Can't init char buf");
		freeBuff(buff);
		return NULL;
	}

	memset(buff->buf, 0, buff->max);
	buff->used = buff->rpos = buff->wpos = 0;
	return buff;
}

public bufferArray* newBuffArr()
{
	bufferArray* arrayBuff = (bufferArray*)malloc(sizeof(bufferArray));
	if (!arrayBuff) {
		debug(DEBUG_ERROR, "Can't create bufferArray");
		return NULL; 
	}

	arrayBuff->max = PER_BUF_ARRAY_SIZE;
	arrayBuff->buff = (buffer**)malloc(sizeof(buffer*) * arrayBuff->max);
	if (!arrayBuff->buff) {
		debug(DEBUG_ERROR, "Can't init buf array");
		freeArrayBuff(arrayBuff);
		return NULL;
	}

	memset(arrayBuff->buff, 0, arrayBuff->max);
	arrayBuff->used = arrayBuff->rpos = arrayBuff->wpos = 0;
	arrayBuff->pendding = false;
	return arrayBuff;
}

public buffer* resizeBuff(buffer* buff, int64_t addSize)
{
	if (!buff || addSize == 0) {
		return buff;
	}

	char* newBuf = realloc(buff->buf, buff->max + addSize);
	if (!newBuf) {
		debug(DEBUG_ERROR, "Can't resize the buffer[add: %d]", addSize);
		return NULL;
	}

	if (addSize > 0) {
		memset(buff->buf + buff->max, 0, addSize);
	}

	buff->max += addSize;
	buff->buf = newBuf;
	return buff;
}

public void srangeBuff(buffer* buff, size_t current)
{
	if (!buff) {
		return;
	}
 
	if (!buff->buf) {
		buff->used = buff->rpos = buff->wpos = 0;
		return;
	}

	size_t tmpCurrentPos = current;
	buff->rpos = 0;
	if (tmpCurrentPos == 0) {
		tmpCurrentPos = buff->rpos;
	} else if (tmpCurrentPos < 0) {
		tmpCurrentPos = buff->wpos;
	}

	if (tmpCurrentPos >= buff->wpos) {
		// 全部删除
		buff->wpos = 0;
		buff->used = 0;
		memset(buff->buf, 0, buff->max);
		return;
	}

	buff->wpos -= tmpCurrentPos;
	if (tmpCurrentPos < buff->rpos) {
		buff->rpos -= tmpCurrentPos;
	}

	buff->used -= tmpCurrentPos;
	// 使用move
	memmove(buff->buf, buff->buf + tmpCurrentPos, buff->wpos);
	memset(buff->buf + buff->wpos, 0, buff->max);
}

public boolean isBuffEof(buffer* buff)
{
	return buff->wpos <= buff->rpos ? true : false;
}

public boolean isBuffEnough(buffer* buff, size_t size)
{
	return (buff->max - buff->wpos >= size) ? true : false;
}

public bufferArray* resizeArrayBuff(bufferArray* arrayBuff, int64_t addSize)
{
	if (!arrayBuff || addSize == 0) {
		return arrayBuff;
	}

	if (addSize < 0) {
		int64_t index = addSize;
		for (; index < 0; index++) {
			freeBuff(arrayBuff->buff[arrayBuff->max + index]);
		}
	}

	buffer** newBuff = realloc(arrayBuff->buff, arrayBuff->max + addSize);
	if (!newBuff) {
		debug(DEBUG_ERROR, "Can't resize the buffer[add: %d]", addSize);
		return NULL;
	}

	if (addSize > 0) {
		memset(arrayBuff->buff + arrayBuff->max, 0, addSize);
	}

	arrayBuff->max += addSize;
	arrayBuff->buff = newBuff;
	return arrayBuff;
}

public void srangeArrayBuff(bufferArray* arrayBuff, size_t current)
{
	if (!arrayBuff) {
		return;
	}
 
	if (!arrayBuff->buff) {
		arrayBuff->used = arrayBuff->rpos = arrayBuff->wpos = 0;
		return;
	}

	size_t tmpCurrentPos = current;
	if (tmpCurrentPos == 0) {
		tmpCurrentPos = arrayBuff->rpos;
	} else if (tmpCurrentPos < -1) {
		tmpCurrentPos = arrayBuff->wpos;
	}

	size_t bufferLen = sizeof(buffer);
	arrayBuff->rpos = 0;
	buffer* currentBuff = NULL;
	if (tmpCurrentPos >= arrayBuff->wpos) {
		// 全部删除
		// 这里判断比最大值小是为了防止乱用
		while (arrayBuff->wpos && arrayBuff->wpos <= arrayBuff->max) {
			buffer* currentBuff = *(arrayBuff->buff + arrayBuff->wpos - 1);
			if (currentBuff && currentBuff->used > 0) {
				freeBuff(currentBuff);
			}
			
			arrayBuff->wpos--;
			arrayBuff->buff[arrayBuff->wpos] = NULL;
		}

		arrayBuff->wpos = 0;
		arrayBuff->used = 0;
		return;
	}

	arrayBuff->wpos -= tmpCurrentPos;
	if (tmpCurrentPos < arrayBuff->rpos) {
		arrayBuff->rpos -= tmpCurrentPos;
	}

	arrayBuff->used -= tmpCurrentPos;
	// 使用move
	memmove(
		arrayBuff->buff,
		arrayBuff->buff + tmpCurrentPos,
		bufferLen * arrayBuff->wpos
	);
	memset(arrayBuff->buff + arrayBuff->wpos, 0, arrayBuff->max);
}

public boolean isArrayBuffEof(bufferArray* arrayBuff)
{
	return arrayBuff->wpos <= arrayBuff->rpos ? true : false;
}

public boolean isArrayBuffEnough(bufferArray* arrayBuff, size_t size)
{
	return (arrayBuff->max - arrayBuff->wpos >= size) ? true : false;
}

public void freeBuff(buffer* buff)
{
	if (buff) {
		if (buff->buf) {
			free(buff->buf);
		}

		free(buff);
	}
}

public void freeArrayBuff(bufferArray* arrayBuff)
{
	if (arrayBuff) {
		buffer* currentBuff = NULL;
		while (arrayBuff->wpos > 0) {
			currentBuff = *(arrayBuff->buff + arrayBuff->wpos - 1);
			freeBuff(currentBuff);
			currentBuff = NULL;
			arrayBuff->wpos--;
		}

		arrayBuff->buff = NULL;
		free(arrayBuff);
	}
}

public boolean parseInt(const char* numeric, size_t slen, int64_t* num)
{
	if (!num) {
		return false;
	}

	const char* p = numeric;
	size_t plen = 0;
	boolean negative = false;
	uint64_t n = 0;
	// 字符串不存在
	if (plen == slen) {
		return false;
	}

	// 0
	if (slen == 1 && *p == '0') {
		*num = 0;
		return true;
	}

	// -
	if (*p == '-') {
		p++;
		plen++;
		negative = true;
		if (plen == slen) {
			return false;
		}
	}

	// -0
	if (*p == '0' && plen == 1) {
		*num = 0;
		return true;
	}

	// numeric
	if (*p < '1' || *p > '9') {
		return false;
	}

	n = *p - '0';
	p++;
	plen++;
	while (plen < slen && *p >= '1' && *p <= '9') {
		if (n > ULLONG_MAX / 10) {
			return false;
		}

		n *= 10;
		if (n > ULLONG_MAX - (*p - '0')) {
			return false;
		}

		n += (*p - '0');
		p++;
		plen++;
	}

	if (plen < slen) {
		return false;
	}

	if (negative) {
		if (n > (uint64_t)(-(LLONG_MIN + 1)) + 1) {
			return false;
		}

		*num = -n;
	} else {
		if (n > LLONG_MAX) {
			return false;
		}

		*num = n;
	}

	return true;
}

public byte addBuffer(bufferArray* arrayBuff, size_t alen, const char* add, size_t len)
{
	if (arrayBuff->max <= alen && !resizeArrayBuff(arrayBuff, PER_BUF_ARRAY_SIZE)) {
		// 扩展
		return S_ERR;
	}

	// 只考虑顺序增长
	if (arrayBuff->wpos <= alen) {
		alen = arrayBuff->wpos;
		arrayBuff->wpos++;
		arrayBuff->used++;
	}

	buffer** buff = arrayBuff->buff + alen;
	if (!buff || !(*buff)) {
		buffer* tmp = newBuff();
		if (!tmp) {
			return S_ERR;
		}

		*buff = tmp;
	}

	if (addBuff(*buff, add, len) == S_ERR) {
		return S_ERR;
	}

	return S_OK;
}

public byte addBuff(buffer* buff, const char* add, size_t len)
{
	int32_t extra = len - (buff->max - buff->wpos);
	if (extra > 0) {
		// 此处不做数据量处理
		size_t addSize = PER_IOBUF_SIZE + 1;
		size_t multi = extra / addSize;
		if (multi <= 0) {
			multi = 1;
		}

		addSize *= multi;
		// 扩展
		if (!resizeBuff(buff, addSize)) {
			return S_ERR;
		}

		buff->used += len;
	}

	memcpy(buff->buf + buff->wpos, add, len);
	buff->wpos += len;
	buff->used += len;
	return S_OK;
}

public char* trim(char* s)
{
	if (!s || strlen(s) <= 0) {
		return s;
	}

	char* l = s + strlen(s) - 1;
	while (isspace(*s)) {
		s++;
	}

	while (l > s && isspace(*l)) {
		l--;
	}

	*(++l) = '\0';
	return s;
}

public byte mvArrayBuff(bufferArray* dest, bufferArray* source, boolean srange)
{
	size_t copyNum = source->wpos;
	if (source->pendding) {
		copyNum--;
	}

	if (copyNum < 1) {
		return S_NT;
	}

	// 判断是否需要重新申请
	if (dest->max - dest->wpos < copyNum) {
		size_t multi = copyNum / PER_BUF_ARRAY_SIZE;
		if (multi <= 0) {
			multi = 1;
		}

		if (!resizeArrayBuff(dest, (PER_BUF_ARRAY_SIZE * multi))) {
			return S_ERR;
		}
	}

	size_t index = 0;
	for (; index < copyNum; index++) {
		// 交互数据即可
		buffer* tmp = source->buff[index];
		source->buff[index] = dest->buff[dest->wpos];
		dest->buff[dest->wpos++] = tmp;
		dest->used++;
	}

	if (srange) {
		srangeArrayBuff(source, copyNum);
	}

	return S_OK;
}

public void clearArrayBuff(bufferArray* arrayBuff)
{
	memset(arrayBuff->buff, 0, arrayBuff->max);
	arrayBuff->wpos = arrayBuff->rpos = 0;
	arrayBuff->used -= arrayBuff->wpos;
}