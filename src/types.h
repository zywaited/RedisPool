#ifndef __TYPES_HEADER
#define __TYPES_HEADER

#include "base.h"
#include "debug/debug.h"
#include "map/hash.h"
#include "map/tree.h"

typedef struct global {
	boolean isExit;
	boolean isTcp;
	pid_t pid;
	union {
		char ip[IP_LEN];
		size_t port;
		char path[UNIX_PATH_LEN];
	} sock;
	size_t backlog;
	tree* children;
	struct timeval timeout;
} global;

typedef struct buffer {
	char* buf;
	size_t max;
	size_t rpos;
	size_t wpos;
	size_t used;
} buffer;

typedef struct bufferArray {
	buffer** buff;
	size_t rpos;
	size_t wpos;
	size_t max;
	size_t used;
	boolean pendding;
} bufferArray; // array buffer

public extern global* globals;

/**
 * @access public
 * @param global* globals
 */
public void freeGlobal(global*);

/**
 * @access public
 * @return buffer*
 */
public buffer* newBuff();

/**
 * @access public
 * @return bufferArray*
 */
public bufferArray* newBuffArr();

/**
 * 增加buff的内存
 * @access public
 * @param buffer* buff
 * @param int64_t addSize
 * @return buffer*
 */
public buffer* resizeBuff(buffer*, int64_t); // resize

/**
 * 截取buff并重置索引
 * @access public
 * @param buffer* buff
 * @param size_t current 从当前开始
 */
public void srangeBuff(buffer*, size_t); // remove

/**
 * @param buffer* buff
 * @return boolean
 */
public boolean isBuffEof(buffer*);

/**
 * @param buffer* buff
 * @param size_t size
 * @return boolean
 */
public boolean isBuffEnough(buffer*, size_t);

/**
 * 增加arrayBuff的长度
 * @access public
 * @param bufferArray* arrayBuff
 * @param int64_t addSize
 * @return bufferArray*
 */
public bufferArray* resizeArrayBuff(bufferArray*, int64_t);

/**
 * 截取arrayBuff并重置索引
 * @access public
 * @param bufferArray* arrayBuff
 * @param size_t current 从当前开始
 */
public void srangeArrayBuff(bufferArray*, size_t);

/**
 * @param bufferArray* arrayBuff
 * @return boolean
 */
public boolean isArrayBuffEof(bufferArray*);

/**
 * @param bufferArray* arrayBuff
 * @param size_t size
 * @return boolean
 */
public boolean isArrayBuffEnough(bufferArray*, size_t);

/**
 * @access public
 * @param buffer* buff
 */
public void freeBuff(buffer*);

/**
 * @access public
 * @param bufferArray* arrayBuff
 */
public void freeArrayBuff(bufferArray*);

/**
 * @access public
 * @param const char* numeric
 * @param size_t slen
 * @param int64_t* num
 */
public boolean parseInt(const char*, size_t, int64_t*);

/**
 * @param bufferArray* arrayBuff
 * @param size_t alen
 * @param const char* add
 * @param size_t len
 */
public byte addBuffer(bufferArray*, size_t, const char*, size_t);

/**
 * @param buffer* buff
 * @param const char* add
 * @param size_t len
 */
public byte addBuff(buffer*, const char*, size_t);

/**
 * @param const char* s
 * @return char*
 */
public char* trim(char*);

/**
 * @param bufferArray* dest
 * @param bufferArray* source
 * @param boolean srange
 * @return byte
 */
public byte mvArrayBuff(bufferArray*, bufferArray*, boolean);

public void clearArrayBuff(bufferArray*);

#endif