#ifndef __CMD_H
#define __CMD_H

#include "base.h"
#include "net/s.h"

#define START_CMD "start"

// 子进程重启
#define RESTART_CMD "restart"

#define STOP_CMD "stop"

#define MAX_SIGNAL_LEN 128
typedef struct signalA {
	int signo;
	void (*func)(int);
} signalA;

public boolean iniCmd(dictionary*, int, char**);

public boolean isStart(char*);

public boolean isReStart(char*);

public boolean isStop(char*);

public void stopWorker(int, boolean);

public void stopParentWorker(int);

public void stopChildWorker(int);

public void installSignal(boolean);

public void dealSingal(int);

#ifdef HAVE_SYS_SIGNALFD_H
public void readSg(evutil_socket_t, short, void*);
#endif

#endif