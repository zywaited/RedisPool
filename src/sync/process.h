/**
 * User: zhangjian2
 */

#ifndef __PROCESS_HEADER
#define __PROCESS_HEADER

#include <unistd.h>
#include "interface.h"

#define CHILD_EXIT_OK 0
// 子进程退出类型
#define CHILD_EXIT_NORMAL 0
#define CHILD_EXIT_SIGNAL 1
#define CHILD_EXIT_TRACE 2
#define CHILD_EXIT_UNKNOWN 3

typedef void* (*childExitCallback)(pid_t, byte, int);

public void processJoin(_sync*); // 等待子进程完成

public void processStart(_sync*); // 线程启动

public void processDetach(_sync*); // 进程独立, 不建议使用

public void* processDeal(_sync*); // 中转

public void releaseChild(childExitCallback);

/**
 * @param int argc
 * @param char** argv
 */
public byte saveArgs(int, char**);

/**
 * @param const char* title
 */
public void setProcessTitle(const char*);

#endif