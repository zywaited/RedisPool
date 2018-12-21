/**
 * User: zhangjian2
 */
#ifndef __DEBUG_HEADER
#define __DEBUG_HEADER
#include <unistd.h>
#include "base.h"

#define DEBUG_NOTICE 0
#define DEBUG_WARNING 1
#define DEBUG_INFO 2
#define DEBUG_ERROR 3
#define DEBUG_DEBUG 4

#define DEBUG_OFF 0
#define DEBUG_NORMAL 1
#define DEBUG_FILE 2

#define MAX_DEBUG_LENGTH 1024
#define DEFAULT_TIME_LENGTH 28
#define DEFAULT_INFO_LENGTH 20
#define DEFAULT_PID_LENGTH 10

public void setDebugTypePower(byte, boolean);

public void setDebugMode(byte, const char*);

public void dropDebug(void);

public byte getDebugMode(void);

public void debug(byte, const char*, ...);

/**
 * STDIN_FILENO 0
 * STDOUT_FILENO 1
 * STDERR_FILENO 2
 */
public byte setDup2(const char*, int);

#endif