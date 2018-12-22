#ifndef __MULTIBULK_HEADER
#define __MULTIBULK_HEADER

#include "types.h"
#include "net/s.h"

/**
 * 解析命令集合
 * @param client* c
 * @return byte
 */
public byte processClient(client*);

/**
 * 解析命令字符串
 * @param client* c
 * @param boolean 是否是客户端
 * @return boolean
 */
private byte processCmds(client*, boolean);

/**
 * 解析命令集合
 * @param redis* rs
 * @return byte
 */
public byte processRedis(redis*);

/**
 * 解析数字合法性
 * @param client* c
 * @param int64_t* num
 * @param size_t* num
 * @param byte
 */
private byte parseInputBuffNum(client*, int64_t*, size_t*);

#endif