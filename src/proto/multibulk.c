#include "multibulk.h"
#include "debug/debug.h"
#include "net/pool.h"
#include "net/client.h"
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include "map/hash.h"

public extern public pool* rps;

// 依赖全局变量rps
// 传输发生错误直接返回, 如果是redis返回错误， 关闭redis连接

// 1 协议错误直接根据当前的命令数赋值到回复数组中，不再经过redis
// 2 内容不足也直接复制到命令中，不用再等待所有数据传输完成
public byte processClient(client* c)
{
	int64_t multiBulkLen = 0;
	byte status = S_OK;
	while (!isBuffEof(c->cmd)) {
		if (c->multiBulkLen <= 0) {
			// 未解析过
			// 以*开头
			if (c->cmd->buf[c->cmd->rpos] != '*') {
				debug(DEBUG_ERROR, "[%s]the protocol error, must be start with *", c->ip);
				addReplyCmd(c, "-the protocol error, must be start with *");
				return S_ERR;
			}

			size_t len = 0;
			status = parseInputBuffNum(c, &multiBulkLen, &len);
			if (status == S_ERR) {
				return S_ERR;
			}

			if (status == S_NT) {
				return S_OK;
			}

			c->multiBulkLen = multiBulkLen;
			addReplyLen(c, c->cmd->buf + c->cmd->rpos - len - 3, len + 1);
		}

		status = processCmds(c, true);
		if (status == S_ERR) {
			return S_ERR;
		}

		if (status == S_NT) {
			return S_OK;
		}

		c->multiBulkLen = 0;
	}

	return S_OK;
}

private byte processCmds(client* c, boolean isClient)
{
	int64_t buffLen = 0;
	c->cmds->pendding = true;
	while (c->multiBulkLen) {
		if (c->bulkLen <= 0) {
			// 没有解析过
			// 以$开头
			if (c->cmd->buf[c->cmd->rpos] != '$') {
				debug(DEBUG_ERROR, "[%s]the protocol error, body must be start with $", c->ip);
				addReplyCmd(c, "-the protocol error, body must be start with $");
				return S_ERR;
			}

			size_t len = 0;
			byte status = parseInputBuffNum(c, &buffLen, &len);
			if (status != S_OK) {
				return status;
			}

			if (buffLen <= 0 && isClient) {
				debug(DEBUG_ERROR, "[%s]the protocol error, invalid bulk len", c->ip);
				addReplyCmd(c, "-the protocol error, invalid bulk len");
				return S_ERR;
			}

			if (buffLen > 0) {
				c->bulkLen = buffLen;
			}

			addReplyLen(c, c->cmd->buf + c->cmd->rpos - len - 3, len + 1);
		}

		// 判断数据量是否足够
		if (c->bulkLen > 0) {
			if (c->cmd->wpos - c->cmd->rpos < (size_t)(c->bulkLen + 2)) {
				return S_NT;
			}

			c->cmd->rpos += c->bulkLen;
			if (c->cmd->buf[c->cmd->rpos] != '\r' || c->cmd->buf[c->cmd->rpos + 1] != '\n') {
				debug(DEBUG_ERROR, "[%s]the protocol error, must be end with \\r\\n", c->ip);
				addReplyCmd(c, "-the protocol error, must be end with \\r\\n");
				return S_ERR;
			}

			if (c->argc == 0 && isClient) {
				// 代表命令
				char tmpCmd[60] = {};
				strncpy(tmpCmd, c->cmd->buf + c->cmd->rpos - c->bulkLen, c->bulkLen);
				mapKey s = {0, tmpCmd};
				if (c->isBlock && rps->releaseCmds && isExistKey(rps->releaseCmds, &s)) {
					c->isBlock = false;
				} else if (!c->isBlock && rps->blockCmds && isExistKey(rps->blockCmds, &s)) {
					c->isBlock = true;
				}
			}

			addReplyLen(c, c->cmd->buf + c->cmd->rpos - c->bulkLen, c->bulkLen);
			c->cmd->rpos += 2;
		}

		c->bulkLen = 0;
		c->multiBulkLen--;
		c->argc++;
	}

	c->argc = 0;
	c->cmdNum++;
	c->cmds->pendding = false;
	srangeBuff(c->cmd, c->cmd->rpos);
	return S_OK;
}

public byte processRedis(redis* rs)
{
	client* c = rs->c;
	while (c->rNum) {
		c->cmds->pendding = true;
		int64_t multiBulkLen = 0;
		if (c->multiBulkLen <= 0) {
			char* newLine = strchr(nextReadBuff(c), '\r');
			if (newLine == NULL) {
				if (c->cmd->wpos > MAX_INLINE_SIZE) {
					debug(DEBUG_ERROR, "[%s]too big mbulk count string", c->ip);
					addReplyCmd(c, "-too big mbulk count string");
					return S_ERR;
				}

				return S_OK;
			}

			boolean addReplyType = true;
			switch (*(nextReadBuff(c))) {
				case '+':
				case '-':
					// ok pong
					break;
				case ':':
					if (!parseInt(nextReadBuff(c) + 1, newLine - (nextReadBuff(c) + 1), &multiBulkLen)) {
						debug(DEBUG_ERROR, "[%s]the protocol error, after : must be numeric", c->ip);
						addReplyCmd(c, "-the protocol error, after : must be numeric");
						return S_ERR;
					}

					break;
				case '*':
					if (!parseInt(nextReadBuff(c) + 1, newLine - (nextReadBuff(c) + 1), &multiBulkLen)) {
						debug(DEBUG_ERROR, "[%s]the protocol error, after : must be numeric", c->ip);
						addReplyCmd(c, "-the protocol error, after : must be numeric");
						return S_ERR;
					}

					if (multiBulkLen <= 0) {
						break;
					}

					c->multiBulkLen = multiBulkLen;
					break;
				case '$':
					c->multiBulkLen = 1;
					addReplyType = false;
					break;
				default:
					debug(DEBUG_ERROR, "[%s]the protocol error, unexpected %c", c->ip, *(nextReadBuff(c)));
					addReplyCmd(c, "-the protocol error, unexpected line");
					break;
			}

			if (addReplyType) {
				addReplyBySize(c, nextReadBuff(c), newLine - nextReadBuff(c));
				c->cmd->rpos = newLine - c->cmd->buf + 2;
			}
		}

		if (c->multiBulkLen > 0) {
			byte status = processCmds(c, false);
			if (status == S_ERR) {
				return S_ERR;
			}

			if (status == S_NT) {
				return S_OK;
			}

			c->multiBulkLen = 0;
		}

		c->cmds->pendding = false;
		c->rNum--;
	}

	return S_OK;
}

private byte parseInputBuffNum(client* c, int64_t* num, size_t* len)
{
	char* newLine = strchr(nextReadBuff(c), '\r');
	if (newLine == NULL) {
		if (c->cmd->wpos > MAX_INLINE_SIZE) {
			debug(DEBUG_ERROR, "[%s]too big mbulk count string", c->ip);
			addReplyCmd(c, "-too big mbulk count string");
			return S_ERR;
		}
		
		return S_NT;
	}

	// 规范，只取\n
	if (*(newLine + 1) != '\n') {
		debug(DEBUG_ERROR, "[%s]the protocol error, must be end with \\r\\n", c->ip);
		addReplyCmd(c, "-the protocol error, must be end with \\r\\n");
		return S_ERR;
	}

	size_t slen = newLine - (nextReadBuff(c) + 1);
	if (len) {
		*len = slen;
	}

	// 获取传输长度
	if (!parseInt(nextReadBuff(c) + 1, slen, num)) {
		debug(DEBUG_ERROR, "[%s]the protocol error, after * or $ must be numeric", c->ip);
		addReplyCmd(c, "-the protocol error, after * or $ must be numeric");
		return S_ERR;
	}

	c->cmd->rpos = newLine - c->cmd->buf + 2;
	return S_OK;
}