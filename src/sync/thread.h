/**
 * User: zhangjian2
 */

#ifndef __THREAD_HEADER
#define __THREAD_HEADER

#include <pthread.h>
#include "interface.h"

public void threadJoin(_sync*); // 等待子线程完成

public void threadStart(_sync*); // 线程启动

public void threadDetach(_sync*); // 线程独立

public void* threadDeal(void*); // 中转

#endif