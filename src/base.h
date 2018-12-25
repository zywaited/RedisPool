#ifndef __BASE_H
#define __BASE_H

#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <math.h>
#include "malloc.h"
#include <stdio.h>

#define freeNameLen 30
#define IP_LEN 20
#define UNIX_PATH_LEN 100
#define LISTEN_BACK_LOG 10240
#define MAX_CLIENT_NUM 10240

#define MAX_PORT 65535

/**
 * Access
 */
#define public
#define private static

/**
 * the size of read client or redis
 */
#define PER_IOBUF_SIZE (1024 * 16)
#define PER_MIN_SIZE 128
#define PER_BUF_ARRAY_SIZE 60
#define MAX_QUERY_SIZE (1024 * 1024 * 1024)
#define MAX_INLINE_SIZE (1024 * 64)

typedef unsigned char byte;

/**
 * 用于错误信息， 与boolean区分开
 */
#define S_ERR 0
#define S_OK 1
#define S_NT 2

typedef char boolean;
#define true 1
#define false 0

typedef void (*freeFunc)(void*);

#endif