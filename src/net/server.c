#include "server.h"
#include <string.h>
#include <arpa/inet.h>

#define setTcp(ip, port)										\
	do {														\
		if (strlen(ip) <= 0 || port <= 0 || port > MAX_PORT) {	\
			debug(DEBUG_WARNING, "The ip or port is error");	\
			return -1;											\
		}														\
																\
		if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {		\
			debug(DEBUG_ERROR, "Can't create tcp socket");		\
			return -1;											\
		}														\
																\
		serverIn.sin_family = AF_INET;							\
		serverIn.sin_port = htons(port);						\
		serverIn.sin_addr.s_addr = inet_addr(ip);				\
	} while (0)

#define setUnix(path)														\
	do {																	\
		if (strlen(path) <= 0) {											\
			debug(DEBUG_WARNING, "The ip is error");						\
			return -1;														\
		}																	\
																			\
		if ((fd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {					\
			debug(DEBUG_ERROR, "Can't create unix socket in [%s]", path);	\
			return -1;														\
		}																	\
																			\
		serverUn.sun_family = AF_UNIX;										\
		strcpy(serverUn.sun_path, path);									\
	} while (0)

public void run(int argc, char** argv)
{
	// parse ini and init global and init server and start
	setDebugMode(DEBUG_NORMAL, NULL);
	saveArgs(argc, argv);
	// 没启动不自动释放空间，程序退出
	global* g = (global*)malloc(sizeof(global));
	if (!g) {
		debug(DEBUG_ERROR, "Can't init globals");
		return;
	}

	g->isExit = false;
	g->pid = getpid();
	g->timeout.tv_sec = 0;
	g->timeout.tv_usec = 0;
	globals = g;
	char* execPath = realpath(argv[0], NULL);
	char* configPath = (char*)realloc(execPath, strlen(execPath) + 20);
	char* appPath = dirname(dirname(configPath));
	sprintf(configPath, "%s/%s/%s", appPath, "conf", CONFIG_NAME);
	// 读取配置项
	dictionary* ini = iniparser_load(configPath);
	if (!ini) {
		debug(DEBUG_ERROR, "Can't load config[%s]", configPath);
		return;
	}

	free(configPath);
	debug(DEBUG_INFO, "start to read ini");
	int timeout = iniparser_getint(ini, "global:timeout", 0);
	if (timeout > 0) {
		g->timeout.tv_sec = timeout;
	}

	int maxClientSize = iniparser_getint(ini, "client:max", 0);
	if (maxClientSize <= 0 || maxClientSize > MAX_CLIENT_NUM) {
		maxClientSize = MAX_CLIENT_NUM;
	}

	int millisecond = iniparser_getint(ini, "server:interval", 0);
	if (millisecond <= 0) {
		millisecond = 2;
	}

	int isTcp = iniparser_getint(ini, "server:tcp", 0);
	int backlog = iniparser_getint(ini, "server:backlog", 0);
	if (backlog <= 0 || backlog > LISTEN_BACK_LOG) {
		backlog = LISTEN_BACK_LOG;
	}

	g->backlog = backlog;
	int fd = -1;
	const char* redisIp = iniparser_getstring(ini, "redis:ip", NULL);
	if (!redisIp || strlen(redisIp) < 1) {
		debug(DEBUG_ERROR, "Redis ip not exist");
		return;
	}

	int redisPort = iniparser_getint(ini, "redis:port", 0);
	int redisMax = iniparser_getint(ini, "redis:max", 0);
	int redisMin = iniparser_getint(ini, "redis:min", 0);
	if (redisPort <= 0 ||
		redisMax < 0 ||
		redisMin < 0 ||
		redisMin > redisMax
	) {
		debug(DEBUG_ERROR, "Redis config error");
		return;
	}

	debug(DEBUG_INFO, "start to create serer socket");
	if (isTcp > 0) {
		g->isTcp = true;
		const char* ip = iniparser_getstring(ini, "server:ip", NULL);
		if (!ip || strlen(ip) < 1) {
			debug(DEBUG_ERROR, "Server ip is error");
			return;
		}

		int port = iniparser_getint(ini, "server:port", 0);
		if (port < 1) {
			debug(DEBUG_ERROR, "Server port is error");
			return;
		}

		fd = getTcpFd(ip, port, backlog);
		strcpy(g->sock.ip, ip);
		g->sock.port = port;
	} else {
		g->isTcp = false;
		const char* path = iniparser_getstring(ini, "server:path", NULL);
		if (!path || strlen(path) < 1) {
			debug(DEBUG_ERROR, "Server unix path is error");
			return;
		}

		remove(path);
		fd = getUnixFd(path, backlog);
		strcpy(g->sock.path, path);
	}

	if (fd <= 0) {
		return;
	}

	if (!initServer(fd, maxClientSize, millisecond)) {
		return;
	}

	service->ini = ini;
	int children = iniparser_getint(ini, "server:children", 0);
	if (children <= 0 || children > FORK_MAX_NUM) {
		children = FORK_MAX_NUM;
	}

	g->children = newTree(children, NULL);
	if (!g->children) {
		return;
	}

	if (children > 1) {
		debug(DEBUG_INFO, "start to create pipe sock");
		sock* socks = newSock((uint32_t)children, service->interval);
		if (!socks) {
			return;
		}

		service->socks = socks;
	}

	const char* parentName = iniparser_getstring(ini, "global:parent", NULL);
	setProcessTitle(parentName);
	int exist = 1;
	mapValue p = {&exist, false};
	int index = 0;
	debug(DEBUG_INFO, "start to create child worker");
	for (; index < children; ++index) {
		chan* tmp = NULL;
		if (service->socks) {
			tmp = service->socks->ch + index;
		}

		_sync* sc = newSync(PROCESS, NOT_DETACH, NULL, NULL, worker, (void*)tmp, "worker");
		start(sc);
		treePut(globals->children, (int)(sc->id), &p);
		if (!service->socks) {
			continue;
		}

		mapValue* v = newMapValue((void*)tmp, false);
		if (!v) {
			return;
		}

		treePut(service->socks->pToC, (int)(sc->id), v);
		deleteSync(sc);
	}

	debug(DEBUG_INFO, "start to monitor worker");
	while (true) {
		// 暂不加信号处理, 循环读取
		releaseChild(restartWorker);
		usleep(300000);
		// 判断子进程不存在退出
		if (globals->children->total < 1) {
			debug(DEBUG_INFO, "the children all exit, now to end");
			return;
		}
	}
}

public boolean initServer(int fd, size_t maxClientSize, size_t millisecond)
{
	server* s = (server*)malloc(sizeof(server));
	if (!s) {
		return false;
	}

	service = s;
	s->fd = fd;
	s->pid = 0;
	s->clients = NULL;
	s->maxClientSize = maxClientSize;
	s->interval.tv_sec = (time_t)millisecond;
	s->interval.tv_usec = 0;
	s->rps = NULL;
	s->cToR = s->rToC = NULL;
	s->socks = NULL;
#ifdef USE_EPOLL
	// s->createBaseEvent = createEpBase;
	// s->addEvent = epAdd;
	// s->loopEvent = epLoop;
	// s->freeEvent = freeEpEvent;
#else
	s->createBaseEvent = createLibBase;
	s->addEvent = libAdd;
	s->loopEvent = libLoop;
	s->freeEvent = freeLibFreeEvent;
#endif
	s->base = NULL;
	s->accept = NULL;
	return true;
}

public void freeServer(server* s)
{
	if (!s) {
		return;
	}

	if (s->clients) {
		boolean isExitFree = true;
		if (!s->clients->freeFuncName) {
			isExitFree = false;
			s->clients->freeFuncName = freeClientByVoid;
		}
		
		if (!isExitFree) {
			s->clients->freeFuncName = NULL;
		}

		s->clients = NULL;
	}

	if (s->rps) {
		freePool(s->rps);
		s->rps = NULL;
	}

	if (s->cToR) {
		freeHash(s->cToR);
		s->cToR = NULL;
	}

	if (s->rToC) {
		freeHash(s->rToC);
		s->rToC = NULL;
	}

	if (s->socks) {
		freeSock(s->socks);
		s->socks = NULL;
	}

	if (s->accept) {
		freeClientE(s->accept);
		s->accept = NULL;
	}

	if (s->ini) {
		iniparser_freedict(s->ini);
		s->ini = NULL;
	}

	free(s);
}

public void readR(evutil_socket_t fd, short flag, void* args)
{
	redis* r = (redis*)args;
	// 解析
	mapKey rk = {(uint64_t)fd, NULL};
	// client
	mapValue* cv = hashGet(service->rToC, &rk);
	client* c = NULL;
	if (cv) {
		c = (client*)cv->value;
	}

	if (!c || !c->isValid || c->isQuit) {
		removeRC(r, &rk, NULL, NULL);
		return;
	}

	gettimeofday(&(r->c->last), NULL);
	mapKey ck = {(uint64_t)c->fd, NULL};
	if (baseRead(r->c) == S_ERR) {
		c->isQuit = true;
		removeRC(r, &rk, NULL, &ck);
		return;
	}

	if (errno == EAGAIN) {
		return;
	}

	if (processRedis(r) == S_ERR) {
		c->isQuit = true;
		removeRC(r, &rk, NULL, &ck);
		return;
	}

	// 写入client
	if (mvArrayBuff(c->middle, r->c->cmds, true) == S_ERR) {
		c->isQuit = true;
		removeRC(r, &rk, NULL, &ck);
		return;
	}

	srangeBuff(r->c->cmd, r->c->cmd->rpos);
	r->c->cmdNum = 0;
	// 判断是否写入完成
	if (r->c->isBlock) {
		return;
	}

	if (r->c->rNum != 0) {
		return;
	}

	r->c->rNum = 0;
	rk.h = (uint64_t)r->c->fd;
	rk.str = NULL;
	ck.h = (uint64_t)c->fd;
	ck.str = NULL;
	hashRemove(service->rToC, &rk);
	hashRemove(service->cToR, &ck);
	putRedis(rps, r);
}

public void readC(evutil_socket_t fd, short flag, void* args)
{
	client* c = (client*)args;
	mapKey ck = {(uint64_t)fd, NULL};
	if (c->isQuit) {
		return;
	}

	gettimeofday(&(c->last), NULL);
	if (baseRead(c) == S_ERR) {
		// redis
		mapValue* r = hashGet(service->cToR, &ck);
		if (r) {
			mapKey rk = {(uint64_t)((redis*)r->value)->c->fd, NULL};
			removeRC((redis*)r->value, &rk, NULL, &ck);
		}
		
		treeRemoveAndFree(service->clients, (int)fd);
		removeRC(NULL, NULL, c, NULL);
		return;
	}

	if (!c->isValid || c->isQuit || errno == EAGAIN) {
		return;
	}

	redis* r = NULL;
	if (c->isBlock) {
		// redis
		mapValue* rv = hashGet(service->cToR, &ck);
		if (!rv) {
			c->isQuit = true;
			return;
		}

		r = (redis*)rv->value;
	} else {
		r = getRedis(rps);
	}

	if (!r) {
		c->isQuit = true;
		addReplyCmd(c, "-Server is Busy");
		return;
	}

	debug(DEBUG_INFO, "start to process client cmds[%d]", fd);
	if (processClient(c) == S_ERR) {
		if (c->isBlock) {
			delRedis(rps, r, false);
		} else {
			putRedis(rps, r);
		}

		c->isQuit = true;
		return;
	}

	// 写入redis
	debug(DEBUG_INFO, "start to copy cmds to redis[%d]", fd);
	if (mvArrayBuff(r->c->middle, c->cmds, true) == S_ERR) {
		if (c->isBlock) {
			delRedis(rps, r, false);
		} else {
			putRedis(rps, r);
		}

		c->isQuit = true;
		return;
	}

	// 加入
	if (!c->isBlock) {
		mapKey* ack = newMapKey((uint64_t)fd, NULL);
		mapKey* ark = newMapKey((uint64_t)r->c->fd, NULL);
		mapValue* rd = newMapValue((void*)r, false);
		mapValue* ct = newMapValue(args, false);
		if (!ack || !ark || !rd || !ct) {
			freeMapValue(rd, NULL);
			freeMapValue(ct, NULL);
			freeMapKey(ack);
			freeMapKey(ark);
			delRedis(rps, r, false);
			c->isQuit = true;
			return;
		}

		hashPutAndFree(service->cToR, ack, rd);
		hashPutAndFree(service->rToC, ark, ct);
	}

	r->c->rNum += c->cmdNum;
	c->cmdNum = 0;
	r->c->isBlock = c->isBlock;
}

public void writeR(evutil_socket_t fd, short flag, void* args)
{
	redis* r = (redis*)args;
	bufferArray* cmds = r->c->middle;
	if (cmds->wpos <= cmds->rpos) {
		// 判断是否需要截取
		if (cmds->wpos > 0) {
			srangeArrayBuff(cmds, cmds->wpos);
		}

		// 判断是否需要重置大小
		if (cmds->max >= PER_BUF_ARRAY_SIZE * 2) {
			resizeArrayBuff(cmds, PER_BUF_ARRAY_SIZE - cmds->max);
		}

		return;
	}

	debug(DEBUG_INFO, "start to write cmds to redis[%d]", fd);
	if (baseWrite(r->c) == S_ERR) {
		mapKey rk = {(uint64_t)fd, NULL};
		removeRC(r, &rk, NULL, NULL);
		mapValue* cv = hashRemove(service->rToC, &rk);
		if (!cv) {
			return;
		}

		client* c = (client*)cv->value;
		c->isQuit = true;
		return;
	}

	// 判断是否需要重置大小
	if (cmds->rpos >= PER_BUF_ARRAY_SIZE) {
		srangeArrayBuff(cmds, PER_BUF_ARRAY_SIZE);
		resizeArrayBuff(cmds, PER_BUF_ARRAY_SIZE - cmds->max);
	}

	if (cmds->wpos == cmds->rpos) {
		clearArrayBuff(cmds);
	}
}

public void writeC(evutil_socket_t fd, short flag, void* args)
{
	client* c = (client*)args;
	bufferArray* cmds = c->middle;
	mapKey ck = {(uint64_t)fd, NULL};
	if (cmds->wpos <= cmds->rpos) {
		// 判断是否要退出
		if (c->isQuit) {
			removeByC(c, &ck);
			treeRemoveAndFree(service->clients, (int)fd);
			return;
		}

		// 判断是否需要截取
		if (cmds->wpos > 0) {
			srangeArrayBuff(cmds, cmds->wpos);
		}

		// 判断是否需要重置大小
		if (cmds->max >= PER_BUF_ARRAY_SIZE * 2) {
			resizeArrayBuff(cmds, PER_BUF_ARRAY_SIZE - cmds->max);
		}

		return;
	}

	if (baseWrite(c) == S_ERR || !c->isValid || (cmds->wpos <= cmds->rpos && c->isQuit)) {
		removeByC(c, &ck);
		treeRemoveAndFree(service->clients, (int)fd);
		return;
	}

	// 判断是否需要重置大小
	if (cmds->rpos >= PER_BUF_ARRAY_SIZE) {
		srangeArrayBuff(cmds, PER_BUF_ARRAY_SIZE);
		resizeArrayBuff(cmds, PER_BUF_ARRAY_SIZE - cmds->max);
	}

	if (cmds->wpos <= cmds->rpos) {
		clearArrayBuff(cmds);
	}

	return;
}

public int getTcpFd(const char* ip, size_t port, size_t backlog)
{
	int fd;
	struct sockaddr_in serverIn;
	setTcp(ip, port);
	setReuseByFd(fd);
	if (bind(fd, (struct sockaddr*)&serverIn, sizeof(serverIn)) < 0) {
		debug(DEBUG_ERROR, "Can't bind tcp socket in [%s]:[%d]", ip, port);
		close(fd);
		return -1;
	}

	if (listen(fd, backlog) < 0) {
		debug(DEBUG_ERROR, "Can't listen tcp socket in [%s]:[%d]", ip, port);
		close(fd);
		return -1;
	}

	setTimeOut(fd, &globals->timeout);
	return fd;
}

public int getUnixFd(const char* path, size_t backlog)
{
	int fd;
	struct sockaddr_un serverUn;
	setUnix(path);
	setReuseByFd(fd);
	if (bind(fd, (struct sockaddr*)&serverUn, sizeof(serverUn)) < 0) {
		debug(DEBUG_ERROR, "Can't bind unix socket in [%s]", path);
		close(fd);
		return -1;
	}

	if (listen(fd, backlog) < 0) {
		debug(DEBUG_ERROR, "Can't listen unix socket in [%s]", path);
		close(fd);
		return -1;
	}

	setTimeOut(fd, &globals->timeout);
	return fd;
}

public void checkC(evutil_socket_t fd, short flag, void* args)
{
	// todo 后续实现，关闭长时间没有交互的client、redis
}

public void acceptC(evutil_socket_t fd, short flag, void* args)
{
	struct sockaddr addr;
	socklen_t sockaddrLen = sizeof(struct sockaddr);
	int clientFd = accept(fd, &addr, &sockaddrLen);
	if (clientFd <= 0) {
		return;
	}

	char* ip = NULL;
	if (addr.sa_family == AF_INET) {
		struct sockaddr_in clientIp;
		if (getpeername(clientFd, (struct sockaddr*)&clientIp, &sockaddrLen) == 0) {
			ip = inet_ntoa(clientIp.sin_addr);
		} else {
			debug(DEBUG_WARNING, "can't get client's ip[%d]", clientFd);
		}
	} else {
		ip = "127.0.0.1";
	}

	debug(DEBUG_INFO, "server accept a client[%d][%s]", clientFd, ip);
	// 判断是否该进程已到负载
	if (rps->activeNum >= rps->maxNum || service->clients->total >= service->maxClientSize) {
		if (service->socks->used >= MAX_INFO_NUM) {
			debug(DEBUG_WARNING, "Too mush connection[%d]", clientFd);
			send(clientFd, STR_N("-Too mush connection"), 0);
			close(clientFd);
			return;
		}

		// 交由其他进程处理
		clientInfo* info = service->socks->info + service->socks->used;
		info->fd = fd;
		if (ip) {
			strcpy(info->ip, ip);
		} else {
			memset(info->ip, 0, IP_LEN);
		}

		service->socks->used++;
		return;
	}

	client* c = newClient(clientFd, ip, readC, writeC);
	mapValue* nodeValue = newMapValue((void*)c, false);
	if (!c && !nodeValue) {
		debug(DEBUG_ERROR, "Create client error[%d][%s]", clientFd, ip);
		freeClient(c);
		freeMapValue(nodeValue, NULL);
		close(clientFd);
		return;
	}

	treePut(service->clients, clientFd, nodeValue);
}

public int connectTcpFd(char* ip, size_t port)
{
	int fd;
	struct sockaddr_in serverIn;
	setTcp(ip, port);
	if (connect(fd, (struct sockaddr*)&serverIn, sizeof(serverIn)) < 0) {
		debug(DEBUG_ERROR, "Can't connect to socker[%d][%s:%d]", fd, ip, port);
		close(fd);
		return -1;
	}

	setTimeOut(fd, &globals->timeout);
	return fd;
}

public int connectUnixFd(char* path)
{
	int fd;
	struct sockaddr_un serverUn;
	setUnix(path);
	if (connect(fd, (struct sockaddr*)&serverUn, sizeof(serverUn)) < 0) {
		debug(DEBUG_ERROR, "Can't connect to unix[%d][%s]", fd, path);
		close(fd);
		return -1;
	}

	setTimeOut(fd, &globals->timeout);
	return fd;
}

public boolean setTimeOut(int fd, struct timeval* timeout)
{
	if (fd <= 0 || timeout->tv_sec <= 0 && timeout->tv_usec <= 0) {
		return true;
	}

	int ret = setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char*)timeout, sizeof(struct timeval));
	if (ret < 0) {
		debug(DEBUG_WARNING, "set socket[%d] send timeout error", fd);
		return false;
	}

	ret = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char*)timeout, sizeof(struct timeval));
	if (ret < 0) {
		debug(DEBUG_WARNING, "set socket[%d] recv timeout error", fd);
		return false;
	}
}

public void removeRC(redis* r, mapKey* rk, client* c, mapKey* ck)
{
	if (rk) {
		hashRemoveAndFree(service->rToC, rk);
	}

	if (ck) {
		hashRemoveAndFree(service->cToR, ck);
	}

	if (r) {
		delRedis(rps, r, false);
	}

	if (c) {
		freeClient(c);
	}
}

public void removeByR(redis* r, mapKey* rk)
{
	if (r) {
		delRedis(rps, r, false);
	}

	if (!rk) {
		return;
	}

	mapValue* cv = hashRemove(service->rToC, rk);
	if (!cv) {
		return;
	}

	client* c = (client*)cv->value;
	mapKey ck = {(uint64_t)c->fd, NULL};
	hashRemoveAndFree(service->cToR, &ck);
	freeClient(c);
	freeMapValue(cv, NULL);
}

public void removeByC(client* c, mapKey* ck)
{
	if (c) {
		freeClient(c);
	}

	if (!ck) {
		return;
	}

	mapValue* rv = hashRemove(service->cToR, ck);
	if (!rv) {
		return;
	}

	redis* r = (redis*)rv->value;
	mapKey rk = {(uint64_t)r->c->fd, NULL};
	hashRemoveAndFree(service->rToC, &rk);
	delRedis(rps, r, false);
	freeMapValue(rv, NULL);
}

public void readS(evutil_socket_t fd, short flag, void* arg)
{
	sockData sd = {};
	size_t sockLen = sizeof(sockData);
	int nRead = recv((int)fd, &sd, sockLen, 0);
	if (sockLen != (size_t)nRead) {
		debug(DEBUG_ERROR, "Reading from process error[len][%d]", fd);
		return;
	}

	if (nRead < 0) {
		if (errno == EAGAIN) {
			return;
		}

		debug(DEBUG_ERROR, "Reading from process error[%d]", fd);
		// todo 删除监控
		return;
	}

	if (nRead == 0) {
        debug(DEBUG_INFO, "Process closed connection[%d]", fd);
        // todo 删除监控
        return;
	}

	mapValue* mv = NULL;
	if (sd.type == SOCK_CH_FD) {
		int clientFd = (int)sd.data;
		mv = treeGet(service->clients, clientFd);
		if (mv) {
			return;
		}

		// 判断是否该进程已到负载
		if (rps->activeNum >= rps->maxNum || service->clients->total >= service->maxClientSize) {
			debug(DEBUG_WARNING, "Too mush connection[%d]", clientFd);
			send(clientFd, STR_N("-Too mush connection"), 0);
			close(clientFd);
			return;
		}

		client* c = newClient(clientFd, sd.ip, readC, writeC);
		mapValue* nodeValue = newMapValue((void*)c, false);
		if (!c && !nodeValue) {
			debug(DEBUG_ERROR, "Create client error[%d][%s]", clientFd, sd.ip);
			freeClient(c);
			freeMapValue(nodeValue, NULL);
			close(clientFd);
			return;
		}

		treePut(service->clients, clientFd, nodeValue);
		return;
	}

	uint32_t redisNum = sd.data;
	mapKey mk = {(uint64_t)fd, NULL};
	mv = hashGet(service->socks->rN, &mk);
	if (mv) {
		uint32_t* n = (uint32_t*)mv->value;
		*n = redisNum;
		updateMin();
		return;
	}

	mv = treeGet(service->socks->rC, (int)fd);
	if (!mv) {
        // todo 删除监控
        return;
	}

	// 加入数据
	uint32_t* n = (uint32_t*)malloc(sizeof(uint32_t));
	if (!n) {
		return;
	}

	mapKey* k = newMapKey((uint64_t)fd, NULL);
	if (!k) {
		free(n);
		return;
	}

	*n = redisNum;
	mv = newMapValue((void*)n, true);
	if (!mv) {
		free(n);
		freeMapKey(k);
		return;
	}

	hashPutAndFree(service->socks->rN, k, mv);
	updateMin();
}

public void writeS(evutil_socket_t fd, short flag, void* arg)
{
	sockData sd = {};
	size_t sockLen = sizeof(sockData);
	if (fd < 0) {
		chan* ch = (chan*)arg;
		time_t c;
		time(&c);
		if (c - ch->ct.tv_sec < service->socks->interval.tv_sec) {
			return;
		}

		// 定时时间传输当前连接数
		sd.type = SOCK_CH_FD;
		sd.data = rps->activeNum + rps->avalidNum;
		send((int)fd, &sd, sockLen, 0);
		gettimeofday(&ch->ct, NULL);
		return;
	}

	if (service->socks->used <= 0 || !service->socks->min || service->socks->min->w != (int)fd) {
		return;
	}

	clientInfo* info = service->socks->info + 0;
	if (info->fd <= 0) {
		return;
	}

	sd.type = SOCK_CH_RN;
	sd.data = info->fd;
	strcpy(sd.ip, info->ip);
	if (send((int)fd, &sd, sockLen, 0) != sockLen) {
		send(info->fd, STR_N("-Server is busy"), 0);
		close(info->fd);
		return;
	}

	service->socks->used--;
	memmove(service->socks->info, service->socks->info + 1, MAX_INFO_NUM - 1);
	info->fd = -1;
	service->socks->minNum++;
}

public void updateMin()
{
	mapKey* rk = NULL;
	mapValue* rn = NULL;
	int read = 0;
	int tmp = 0;
	uint32_t minNum = service->socks->minNum;
	FOREACH(service->socks->rN, rk, rn)
		uint32_t* num = (uint32_t*)rn->value;
		tmp = (int)rk->h;
		if (tmp == service->socks->min->r) {
			*num = service->socks->minNum;
			continue;
		}

		if (*num > minNum) {
			continue;
		}

		read = tmp;
		minNum = *num;
	END_FOREACH()
	if (read == 0) {
		return;
	}

	mapValue* ch = treeGet(service->socks->rC, read);
	if (!ch) {
		return;
	}

	service->socks->minNum = minNum;
	service->socks->min = (chan*)ch->value;
}

public sock* newSock(uint32_t num, struct timeval interval)
{
	sock* socks = (sock*)malloc(sizeof(sock));
	if (!socks) {
		goto INIT_ERROR;
	}

	socks->ch = (chan*)malloc(sizeof(chan) * num);
	if (!socks->ch) {
		goto INIT_ERROR;
	}

	socks->chNum = num;
	chan* tmp = NULL;
	if (interval.tv_sec > 0) {
		interval.tv_sec = (time_t)ceil((interval.tv_sec / 2.0));
	}

	socks->interval = interval;
	int sv[2] = {};
	uint32_t index = 0;
	for (; index < num; ++index) {
		tmp = socks->ch + index;
		tmp->r = tmp->w = -1;
		if (pipe(sv) < 0) {
			goto INIT_ERROR;
		}

		tmp->r = sv[0];
		tmp->w = sv[1];
		setNonBlockByFd(tmp->r);
		setNonBlockByFd(tmp->w);
		gettimeofday(&tmp->ct, NULL);
		tmp->read = newClientE(readS, EV_READ | EV_PERSIST, (void*)tmp, NULL);
		tmp->write = newClientE(writeS, EV_WRITE | EV_PERSIST, (void*)tmp, NULL);
		if (!tmp->read || !tmp->write) {
			goto INIT_ERROR;
		}
	}

	socks->min = NULL;
	socks->minNum = 0;
	socks->info = (clientInfo*)malloc(sizeof(clientInfo) * MAX_INFO_NUM);
	if (!socks->info) {
		goto INIT_ERROR;
	}

	socks->used = 0;
	socks->pToC = newTree(num, NULL);
	if (!socks->pToC) {
		goto INIT_ERROR;
	}

	socks->rC = newTree(num, NULL);
	if (!socks->rC) {
		goto INIT_ERROR;
	}

	newHash(&socks->rN, NULL);
	if (!socks->rN) {
		goto INIT_ERROR;
	}

	return socks;
INIT_ERROR:
	debug(DEBUG_ERROR, "Init sock error");
	freeSock(socks);
	return NULL;
}

public void freeSock(sock* socks)
{
	if (!socks) {
		return;
	}

	if (socks->ch) {
		chan* tmp = NULL;
		uint32_t index = 0;
		for (; index < socks->chNum; ++index) {
			tmp = socks->ch + index;
			freeClientE(tmp->read);
			freeClientE(tmp->write);
			if (tmp->r) {
				close(tmp->r);
				tmp->r = -1;
			}

			if (tmp->w) {
				close(tmp->w);
				tmp->w = -1;
			}
		}

		free(socks->ch);
	}

	if (socks->info) {
		free(socks->info);
	}

	if (socks->pToC) {
		freeTree(socks->pToC);
	}

	if (socks->rC) {
		freeTree(socks->rC);
	}

	if (socks->rN) {
		freeHash(socks->rN);
	}

	free(socks);
}

public void* worker(cid_t pid, char* name, void* arg)
{
	dictionary* ini = service->ini;
	const char* childName = iniparser_getstring(ini, "global:child", NULL);
	setProcessTitle(childName);
	service->pid = getpid();
	// Linux 2.6.8 参数被忽略，libevent也不需要该参数
	service->base = service->createBaseEvent(MAX_CLIENT_NUM);
	if (!service->base) {
		exit(-1);
	}

	// 学习目的使用,单独开线程读取
	// 处理请求和处理数据分开
	// service->accept = newClientE(acceptC, EV_READ | EV_PERSIST, NULL, NULL);
	// if (!service->accept) {
	// 	return NULL;
	// }

	service->clients = newTree(service->maxClientSize, NULL);
	if (!service->clients) {
		exit(-1);
	}

	newHash(&service->cToR, NULL);
	newHash(&service->rToC, NULL);
	if (!service->cToR || !service->rToC) {
		exit(-1);
	}

	const char* redisIp = iniparser_getstring(ini, "redis:ip", NULL);
	const char* redisAuth = iniparser_getstring(ini, "redis:auth", NULL);
	const char* redisBlock = iniparser_getstring(ini, "redis:block", NULL);
	const char* redisRelease = iniparser_getstring(ini, "redis:release", NULL);
	int redisPort = iniparser_getint(ini, "redis:port", 0);
	int redisMax = iniparser_getint(ini, "redis:max", 0);
	int redisMin = iniparser_getint(ini, "redis:min", 0);
	debug(DEBUG_INFO, "worker start to init redis pool");
	rps = initPool(
		(uint32_t)redisMax,
		(uint32_t)redisMin,
		POOL_NORMAL,
		redisIp,
		redisPort,
		redisAuth,
		readR,
		writeR
	);
	if (!rps) {
		exit(-1);
	}

	setCmds(rps, redisBlock, redisRelease);
	iniparser_freedict(ini);
	service->ini = NULL;
	service->rps = rps;
	// 获取sock， 加入循环中
	// 开线程接受请求
	if (service->socks) {
		debug(DEBUG_INFO, "worker start to add pipe sock event");
		chan* ch = (chan*)arg;
		chan* tmp = NULL;
		uint32_t index = 0;
		for (; index < service->socks->chNum; ++index) {
			tmp = service->socks->ch + index;
			if (tmp->r == ch->r && tmp->w == ch->w) {
				service->addEvent(service->base, ch->r, ch->read);
				freeClientE(ch->write);
				ch->write = NULL;
				continue;
			}

			service->addEvent(service->base, tmp->w, tmp->write);
			freeClientE(tmp->read);
			tmp->read = NULL;
		}
	}

	debug(DEBUG_INFO, "worker start to create accept task");
	_sync* sc = newSync(THREAD, DETACH, NULL, NULL, task, NULL, "accept_task");
	if (!sc) {
		exit(-1);
	}

	start(sc);
	service->loopEvent(service->base, 0);
	return NULL;
}

public void* task(cid_t pid, char* name, void* arg)
{
	debug(DEBUG_INFO, "server start to accept client");
	while (true) {
		acceptC((evutil_socket_t)service->fd, EV_READ | EV_PERSIST, NULL);
	}
	
	return NULL;
}

public void* restartWorker(pid_t pid, byte type, int code)
{
	mapValue* p = treeRemove(globals->children, (int)pid);
	void* tmpCh = NULL;
	if (service->socks) {
		mapValue* v = treeRemove(service->socks->pToC, (int)pid);
		if (v) {
			tmpCh = v->value;
		} else {
			debug(DEBUG_WARNING, "Can't find sock");
		}
	}

	// 非正常退出
	if (type == CHILD_EXIT_NORMAL && code != 0) {
		return NULL;
	}

	// 管道损坏
	if (type = CHILD_EXIT_SIGNAL && code == 13) {
		return NULL;
	}

	_sync* sc = newSync(PROCESS, NOT_DETACH, NULL, NULL, worker, tmpCh, "worker");
	start(sc);
	if (!p) {
		int exist = 1;
		mapValue tmp = {&exist, false};
		p = &tmp;
	}

	treePut(globals->children, (int)(sc->id), p);
	return NULL;
}