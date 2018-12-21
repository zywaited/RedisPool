#ifndef __POOL_HEADER
#define __POOL_HEADER

#include "client.h"
#include <pthread.h>
#include "base.h"
#include "s.h"

#define POOL_NORMAL 0
// not send when client quit
#define BREAK_WITH_OUT 1
// send when client quit
#define CONTINUE_WITH_OUT 2

/**
 * @access public
 * 后期可把参数合并为一个结构体
 * @param uint32_t maxNum
 * @param uint32_t minNum
 * @param byte sendType
 * @param const char* ip
 * @param size_t port
 * @param const char* auth
 * @return pool*
 */
public pool* initPool(
	uint32_t, uint32_t, byte, 
	const char*, size_t, const char*,
	eventCallback, eventCallback
);

/**
 * @access public
 * @param pool* rps
 */
public void freePool(pool*);

/**
 * @access private
 * @param pool* rps
 * @return redis*
 */
private redis* newRedis(pool*);

/**
 * @access public
 * @param pool* rps
 * @return redis
 */
public redis* getRedis(pool*);

/**
 * @access public
 * @param pool* rps
 * @param redis* rd
 */
public void putRedis(pool*, redis*);

public void freeRedis(pool*, redis*);

/**
 * @param pool* rps
 * @param char* cmd
 */
public boolean isBlockCmd(pool*, char*);

/**
 * @param pool* rps
 * @param char* cmd
 */
public boolean isReleaseCmd(pool*, char*);

/**
 * @access public
 * @param pool* rps
 * @param redis* rd
 * @param boolean all
 */
public void delRedis(pool*, redis*, boolean);

/**
 * check process exit and rediss timeout
 * @param evutil_socket_t fd
 * @param short flag
 * @param void* args
 */
public void checkR(evutil_socket_t, short, void*);

/**
 * @param pool* rps
 * @param const char* blockCmds
 * @param const char* releaseCmds
 */
public void setCmds(pool*, const char*, const char*);

#endif