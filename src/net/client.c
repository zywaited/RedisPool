#include "client.h"
#include "string.h"
#include <fcntl.h>

public extern server* service;

public clientE* newClientE(eventCallback func, short flag, void* args, struct timeval* time)
{
	clientE* ce = (clientE*)malloc(sizeof(clientE));
	if (!ce) {
		debug(DEBUG_ERROR, "Can't create clientE");
		return NULL;
	}

	ce->func = func;
	ce->flag = flag;
	ce->args = args;
	ce->time = time;
	ce->e = NULL;
	return ce;
}

public client* newClient(
	int fd,
	char* ip,
	eventCallback read,
	eventCallback write)
{
	client* c = (client*)malloc(sizeof(client));
	if (!c) {
		debug(DEBUG_ERROR, "Can't create client");
		return NULL;
	}

	c->fd = fd;
	c->cmd = newBuff();
	c->cmds = newBuffArr();
	c->middle = newBuffArr();
	c->isEof = false;
	memset(c->ip, 0, IP_LEN); 
	if (ip) {
		strcpy(c->ip, ip);
	}

	gettimeofday(&(c->last), NULL);
	c->read = c->write = NULL;
	setNonBlock(c);
	if (read) {
		c->read = newClientE(read, EV_READ | EV_PERSIST, (void*)c, NULL);
		service->addEvent(service->base, fd, c->read);
	}

	if (write) {
		c->write = newClientE(write, EV_WRITE | EV_PERSIST, (void*)c, NULL);
		service->addEvent(service->base, fd, c->write);
	}

	c->isValid = true;
	c->isQuit = false;
	c->argc = 0;
	c->cmdNum = c->rNum = 0;
	return c;
}

public void freeClientE(clientE* ce)
{
	if (!ce) {
		return;
	}
	
	if (ce->e) {
		service->freeEvent(ce->e);
	}

	free(ce);
}

public void freeClient(client* c)
{
	if (c) {
		freeBuff(c->cmd);
		freeArrayBuff(c->cmds);
		freeClientE(c->read);
		freeClientE(c->write);
		free(c);
		if (c->fd > 0) {
			close(c->fd);
		}
	}
}

public void freeClientByVoid(void* c)
{
	freeClient((client*)c);
}

public byte baseRead(client* c)
{
	int readLen, nRead;
	// 减少内存申请，此处直接判断
	readLen = PER_IOBUF_SIZE;
	if ((c->cmd->max - c->cmd->wpos < readLen) && !resizeBuff(c->cmd, readLen)) {
		return S_ERR;
	}

	// socket还是习惯用recv|send
	nRead = recv(c->fd, c->cmd->buf + c->cmd->wpos, readLen, 0);
	if (nRead < 0) {
		if (errno == EAGAIN) {
			return S_OK;
		}

		debug(DEBUG_ERROR, "Reading from client error[%d]", c->fd);
		return S_ERR;
	}

	if (nRead == 0) {
        debug(DEBUG_INFO, "Client closed connection[%d]", c->fd);
        return S_ERR;
	}

	// 判断是否数据量太大
	size_t readBuffLen = c->cmd->wpos + nRead;
	if (readBuffLen >= MAX_QUERY_SIZE) {
		debug(DEBUG_INFO, "Reading too much[%d]", c->fd);
		return S_ERR;
	}

	c->cmd->wpos = readBuffLen;
	if (readBuffLen > c->cmd->used) {
		c->cmd->used = readBuffLen;
	}

	return S_OK;
}

public byte baseWrite(client* c)
{
	bufferArray* cmds = c->middle;
	int totalLen = 0;
	// 用于判断缓冲区是否空闲
	boolean isEnough = true;
	do {
		if (cmds->used == 0 || cmds->rpos >= cmds->wpos || cmds->rpos >= cmds->used) {
			debug(DEBUG_WARNING, "Write cmds is empty[%d]", c->fd);
			return S_OK;
		}

		buffer* cmd = *(cmds->buff + cmds->rpos);
		int writeLen = cmd->rpos - cmd->wpos;
		if (cmd->used == 0 || writeLen <= 0 || cmd->rpos >= cmd->used) {
			debug(DEBUG_WARNING, "Write cmds is empty[%d]", c->fd);
			return S_OK;
		}

		if (writeLen >= PER_IOBUF_SIZE) {
			writeLen = PER_IOBUF_SIZE;
		}

		int wLen = send(c->fd, cmd->buf + cmd->rpos, writeLen, 0);
		if (wLen < 0) {
			if (errno == EAGAIN) {
				return S_OK;
			}

			debug(DEBUG_ERROR, "Writing to client error[%d]", c->fd);
			return S_ERR;
		}

		if (wLen == 0) {
	        debug(DEBUG_INFO, "Client closed connection[%d]", c->fd);
	        return S_ERR;
		}

		totalLen += wLen;
		if (wLen < writeLen) {
			isEnough = false;
			cmd->rpos += wLen;
			continue;
		}

		// 判断写入大小
		boolean over = true;
		size_t extra = wLen - PER_IOBUF_SIZE;
		if (extra > 0) {
			over = false;
		}

		if (over) {
			// 清空数据
			memset(cmd->buf, 0, cmd->max);
			cmd->used = cmd->rpos = cmd->wpos = 0;
			cmds->rpos++;
			continue;
		}

		srangeBuff(cmd, wLen);
		resizeBuff(cmd, PER_IOBUF_SIZE - cmd->max);
	} while (isEnough && PER_IOBUF_SIZE - totalLen >= PER_MIN_SIZE);
	return S_OK;
}

public byte block(client* c, boolean isBlock)
{
	if (!c || c->fd <= 0) {
		return S_OK;
	}

	int flag;
	if ((flag = fcntl(c->fd, F_GETFL)) < 0) {
		debug(DEBUG_ERROR, "Can't call get fcntl flag");
		return S_ERR;
	}

	if (isBlock) {
		flag &= ~O_NONBLOCK;
	} else{
		flag |= O_NONBLOCK;
	}

	if (fcntl(c->fd, F_SETFL, flag) < 0) {
		debug(DEBUG_ERROR, "Can't call set fcntl flag");
		return S_ERR;
	}

	return S_OK;
}

public byte setNonBlock(client* c)
{
	return block(c, false);
}

public byte setBlock(client* c)
{
	return block(c, true);
}

public byte setNonBlockByFd(int fd)
{
	client c = {};
	c.fd = fd;
	return block(&c, false);
}

public byte setBlockByFd(int fd)
{
	client c = {};
	c.fd = fd;
	return block(&c, true);
}

public byte setReuse(client* c) {
	if (!c || c->fd < 0) {
		return S_OK;
	}

	int yes = 1;
	if (setsockopt(c->fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
		debug(DEBUG_ERROR, "setsockopt SO_REUSEADDR");
		return S_ERR;
	}

#ifdef SO_REUSEPORT
	if (setsockopt(c->fd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes)) < 0) {
		debug(DEBUG_ERROR, "setsockopt SO_REUSEPORT");
		return S_ERR;
	}
#endif
	return S_OK;
}

public byte setReuseByFd(int fd)
{
	client c = {};
	c.fd = fd;
	return setReuse(&c);
}

public void addReplyBySize(client* c, const char* reply, size_t rlen)
{
	addBuffer(c->cmds, c->cmdNum, reply, rlen);
	addBuffer(c->cmds, c->cmdNum, _NL, 2);
}