/**
 * User: zhangjian2
 */

#ifndef __SYNC_INTERFACE_HEADER
#define __SYNC_INTERFACE_HEADER

#include "base.h"

#define MAX_NAME_LENGTH 20
#define THREAD 1
#define PROCESS 2
#define NOT_START 0
#define THREAD_START 1
#define PROCESS_MAIN_START 2
#define PROCESS_CHILD_START 3
#define NOT_DETACH 0
#define DETACH 1

typedef unsigned long int cid_t;

typedef void* (*onStart)(cid_t, char*);
typedef void* (*onEnd)(cid_t, char*, void*);
typedef void* (*onRun)(cid_t, char*, void*);

#define CALL(sc, funcSuffix)  						\
	do {											\
		switch (sc->type) {         				\
			case THREAD:        					\
				thread##funcSuffix(sc); 			\
				break;								\
			case PROCESS:							\
				process##funcSuffix(sc); 			\
				break;								\
			default:								\
				break;								\
		}											\
	} while (0)

typedef struct _sync_args {
	onStart start; // 开启成功时回调，参数为进线程id
	onEnd end; // 结束时回调， 参数为进线程id、执行结果
	onRun run; // 主体函数
	void* run_args; // 主体函数参数
	char  name[MAX_NAME_LENGTH];
} sync_args;

typedef struct _sync {
	cid_t id;
	byte type;
	byte flag;
	byte start;
	sync_args* args;
} _sync;

public void deleteSyncArgs(sync_args*);

public void deleteSync(_sync*);

public _sync* newSync(
	byte,
	byte,
	onStart,
	onEnd,
	onRun,
	void*,
	char*
);

public void join(_sync*);

public void start(_sync*);

public void detach(_sync*);

public byte isValidType(byte);

public void callSync(_sync*);

#endif