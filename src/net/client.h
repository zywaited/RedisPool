#ifndef __CLIENT_HEADER
#define __CLIENT_HEADER

#include "base.h"
#include "event/e.h"
#include <sys/socket.h>
#include "types.h"
#include "s.h"

/**
 * @access public
 * @param eventCallback func
 * @param short flag
 * @param void* args
 * @param struct timeval* time
 * @return clientE*
 */
public clientE* newClientE(eventCallback, short, void*, struct timeval*);

/**
 * @access public
 * @param int fd
 * @return client*
 */
public client* newClient(int, char*, eventCallback, eventCallback);

/**
 * @access public
 * @param clientE* ce
 */
public void freeClientE(clientE*);

/**
 * @access public
 * @param client* c
 */
public void freeClient(client*);

public void freeClientByVoid(void*);

/**
 * @access public
 * @param client* c
 * @return byte
 */
public byte baseRead(client*);

/**
 * @access public
 * @param client* c
 * @return byte
 */
public byte baseWrite(client*);

/**
 * set the fd block status
 * @param client* c
 * @param boolean isBlock
 * @return byte
 */
public byte block(client*, boolean);

public byte setNonBlock(client*);

public byte setBlock(client*);

public byte setNonBlockByFd(int);

public byte setBlockByFd(int);

/**
 * set reuse addr and port
 * @param client* c
 * @return byte
 */
public byte setReuse(client*);

public byte setReuseByFd(int);

/**
 * 添加字符串
 * @param client* c
 * @param const char* reply
 * @param size_t rlen
 */
public void addReplyBySize(client*, const char*, size_t);

#define STR_N(cmd) (cmd),  strlen((cmd))

#define addReply(c, cmd) addReplyBySize((c), (cmd), strlen((cmd)))

#define addReplyLen(c, cmd, len) addReplyBySize((c), (cmd), (len))

#define nextReadBuff(c) ((c)->cmd->buf + (c)->cmd->rpos)

#define addReplyCmdLen(c, cmd, len)		\
	do {								\
		addReplyLen((c), (cmd), (len));	\
		(c)->cmdNum++;					\
	} while (0)

#define addReplyCmd(c, cmd)		\
	do {						\
		addReply((c), (cmd));	\
		(c)->cmdNum++;			\
	} while (0)

#define _NL "\r\n"

#endif