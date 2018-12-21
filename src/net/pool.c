#include "pool.h"
#include <string.h>

public extern server* service;

public pool* initPool(
	uint32_t maxNum, 
	uint32_t minNum,
	byte sendType,
	const char* ip,
	size_t port,
	const char* auth,
	eventCallback read,
	eventCallback write)
{
	if (!ip || strlen(ip) >= IP_LEN || port <= 0 || port > MAX_PORT || 
		maxNum <= 0 || minNum <= 0
	) {
		debug(DEBUG_ERROR, "pool's ip[%s] or port[%d] is error", ip, port);
		return NULL;
	}

	pool* p = (pool*)malloc(sizeof(pool));
	if (!p) {
		debug(DEBUG_ERROR, "init pool[malloc] failed in %s:%d", ip, port);
		return NULL;
	}

	p->maxNum = maxNum;
	p->minNum = minNum;
	p->avalidNum = minNum;
	p->activeNum = 0;
	p->sendType = POOL_NORMAL;
	memset(p->ip, 0, IP_LEN);
	strcpy(p->ip, ip);
	p->port = port;
	memset(p->auth, 0, IP_LEN);
	p->authLen = 0;
	if (auth) {
		size_t authLen = strlen(auth);
		if (authLen > REDIS_AUTH_LEN - 1) {
			authLen = REDIS_AUTH_LEN - 1;
		}

		strncpy(p->auth, auth, authLen);
		p->auth[authLen + 1] = '\0';
		p->authLen = authLen;
	}
	
	p->read = read;
	p->write = write;
	redis* r = NULL;
	redis* tmp = NULL;
	int num = 0;
	for (; num < minNum; num++) {
		tmp = newRedis(p);
		if (!tmp) {
			debug(DEBUG_ERROR, "init pool[redis] failed in %s:%d", ip, port);
			freePool(p);
			return NULL;
		}

		if (!num) {
			r = tmp;
			p->redises = r;
			continue;
		}

		r->next = tmp;
		r = tmp;
	}

	p->blockCmds = p->releaseCmds = NULL;
	return p;
}

public void freePool(pool* rps)
{
	if (!rps) {
		return;
	}

	freeHash(rps->blockCmds);
	freeHash(rps->releaseCmds);
	delRedis(rps, rps->redises, true);
	free(rps);
}

private redis* newRedis(pool* rps)
{
	if (!rps) {
		return NULL;
	}

	int fd = connectTcpFd(rps->ip, rps->port);
	if (fd <= 0) {
		return NULL;
	}

	// 是否需要登陆
	if (rps->authLen > 0) {
		char* auth = (char*)malloc(rps->authLen + 30);
		snprintf(auth, rps->authLen + 30, "*2\r\n$4\r\nauth\r\n$%d\r\n%s\r\n", rps->authLen, rps->auth);
		if (send(fd, auth, strlen(auth), 0) < 0) {
			debug(DEBUG_ERROR, "can't auth the redis socket[%d][%s:%d]", fd, rps->ip, rps->port);
			close(fd);
			free(auth);
			return NULL;
		}
	
		free(auth);
		char ok[10] = {};
		if (recv(fd, ok, 10, 0) < 0 ||
			(ok[0] != '+' && ok[1] != 'O' && ok[2] != 'K')
		) {
			debug(DEBUG_ERROR, "can't auth the redis socket[%d][%s:%d]", fd, rps->ip, rps->port);
			close(fd);
			return NULL;
		}
	}

	redis* r = (redis*)malloc(sizeof(redis));
	if (!r) {
		debug(DEBUG_ERROR, "can't init the redis socket[%d][%s:%d]", fd, rps->ip, rps->port);
		close(fd);
		return NULL;
	}
	r->prev = r->next = NULL;
	r->c = newClient(fd, NULL, NULL, NULL);
	if (!r->c) {
		debug(DEBUG_ERROR, "can't init the redis socket[%d][%s:%d]", fd, rps->ip, rps->port);
		freeRedis(rps, r);
		return NULL;
	}

	if (rps->read) {
		r->c->read = newClientE(rps->read, EV_READ | EV_PERSIST, (void*)r, NULL);
		service->addEvent(service->base, fd, r->c->read);
	}

	if (rps->write) {
		r->c->write = newClientE(rps->write, EV_WRITE | EV_PERSIST, (void*)r, NULL);
		service->addEvent(service->base, fd, r->c->write);
	}

	return r;
}

public redis* getRedis(pool* rps)
{
	if (!rps) {
		return NULL;
	}

	redis* r = NULL;
	if (rps->avalidNum > 0) {
		r = rps->redises;
		rps->redises = r->next;
		rps->avalidNum--;
		rps->activeNum++;
		return r;
	}

	if (rps->maxNum <= rps->activeNum) {
		return NULL;
	}

	// todo 判断最大值
	r = newRedis(rps);
	if (!r) {
		return NULL;
	}

	rps->activeNum++;
	return r;
}

public void putRedis(pool* rps, redis* r)
{
	if (!rps || !r) {
		return;
	}

	redis* rd = rps->redises;
	r->next = rd;
	r->prev = NULL;
	rps->redises = r;
}

public boolean isBlockCmd(pool* rps, char* cmd)
{
	if (!rps || !rps->blockCmds) {
		return false;
	}

	return isExistKey(rps->blockCmds, newMapKey(0, cmd));
}

public boolean isReleaseCmd(pool* rps, char* cmd)
{
	if (!rps || !rps->releaseCmds) {
		return false;
	}

	return isExistKey(rps->releaseCmds, newMapKey(0, cmd));
}

public void freeRedis(pool* rps, redis* r)
{
	if (!rps || !r) {
		return;
	}

	freeClient(r->c);
	r->prev = r->next = NULL;
	free(r);
}

public void delRedis(pool* rps, redis* rd, boolean all)
{
	if (!rps || !rd) {
		return;
	}

	if (!all) {
		if (!rd->prev) {
			rps->redises = rd->next;
		} else {
			rd->prev->next = rd->next;
		}

		freeRedis(rps, rd);
		return;
	}

	redis* tmp;
	redis* next = rd->next;
	redis* prev = rd->prev;
	freeRedis(rps, rd);
	while (prev) {
		tmp = prev->prev;
		freeRedis(rps, prev);
		prev = tmp;
	}

	while (next) {
		tmp = next->next;
		freeRedis(rps, next);
		next = tmp;
	}

	rps->redises = NULL;
}

public void checkR(evutil_socket_t fd, short flag, void* arg)
{

}

public void setCmds(pool* rps, const char* blockCmds, const char* releaseCmds)
{
	// block release 成对
	if (!blockCmds || !releaseCmds) {
		return;
	}

	if (strlen(blockCmds) <= 0 || strlen(releaseCmds) <= 0) {
		return;
	}

	// 用逗号分隔
	newHash(&rps->blockCmds, NULL);
	newHash(&rps->releaseCmds, NULL);
	if (!rps->blockCmds || !rps->releaseCmds) {
		debug(DEBUG_ERROR, "the blockCmd or releaseCmd init error");
		freeHash(rps->blockCmds);
		freeHash(rps->releaseCmds);
		return;
	}

	char* s = strtok((char*)blockCmds, ",");
	while (s) {
		s = trim(s);
		if (strlen(s) > 0) {
			hashPutAndFree(rps->blockCmds, newMapKey(0, s), newMapValue((void*)1, false));
		}

		s = strtok(NULL, ",");
	}

	s = strtok((char*)releaseCmds, ",");
	while (s) {
		s = trim(s);
		if (strlen(s) > 0) {
			hashPutAndFree(rps->releaseCmds, newMapKey(0, s), newMapValue((void*)1, false));
		}
		
		s = strtok(NULL, ",");
	}
}