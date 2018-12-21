#ifndef _S_H
#define _S_H

#include <stdio.h>
#include "base.h"
#include "event/e.h"
#include "iniparser/src/iniparser.h"

#define REDIS_AUTH_LEN 31

typedef struct clientInfo {
	int fd;
	char ip[IP_LEN];
} clientInfo;

typedef struct chan {
	int r;
	int w;
	clientE* read;
	clientE* write;
	struct timeval ct;
} chan;

typedef struct sockData {
	byte type;
	uint32_t data;
	char ip[IP_LEN];
} sockData;

// 管道通信
typedef struct sock {
	chan* ch;
	uint32_t chNum;
	chan* min;
	uint32_t minNum;
	// 预留5个位置
	clientInfo* info;
	int used;
	tree* pToC;
	tree* rC;
	hash* rN;
	struct timeval interval;
} sock;

typedef struct client {
	int fd;
	buffer* cmd;
	bufferArray* cmds;
	bufferArray* middle;
	uint32_t argc;
	boolean isEof;
	char ip[IP_LEN];
	struct timeval last;
	clientE* read;
	clientE* write;
	size_t cmdNum;
	size_t rNum;
	boolean isValid;
	boolean isQuit;
	boolean isBlock; // multi exec
	uint64_t multiBulkLen;
	uint64_t bulkLen;
} client;

typedef struct redis {
	client* c;
	struct redis* prev;
	struct redis* next;
} redis;

typedef struct pool {
    uint32_t maxNum;
    uint32_t minNum;
    uint32_t activeNum;
    uint32_t avalidNum;
	byte sendType; // 预留给检测pool
	char ip[IP_LEN];
	size_t port;
	char auth[REDIS_AUTH_LEN];
	size_t authLen;
    redis* redises;
	hash* blockCmds; // multi
	hash* releaseCmds; // exec
	eventCallback read;
	eventCallback write;
} pool;

typedef struct server {
	/* global.sockId */
	int fd;

	pid_t pid;

	/* clients */
	tree* clients;
	size_t maxClientSize;
	struct timeval interval; // 定时检测

	/* redis */
	pool* rps;

	/* client to redis */
	hash* cToR;

	/* redis to client */
	hash* rToC;

	sock* socks;

	/* event */
	struct event_base* base;
	struct event_base* (*createBaseEvent)(int);
	byte (*addEvent)(struct event_base*, int, clientE*);
	void (*loopEvent)(struct event_base*, byte);
	void (*freeEvent)(struct event*);

	/* 阻塞读取链接，该配置失效 */
	clientE* accept;
	dictionary* ini;
} server;

#endif