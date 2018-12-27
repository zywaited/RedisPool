/**
 * User: zhangjian2
 */
#include "debug.h"
#include <stdarg.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

private byte debugMode = DEBUG_OFF;
private boolean debugAll = true;
private FILE* debugFd = NULL;
public const size_t maxBufLength = DEFAULT_INFO_LENGTH + DEFAULT_TIME_LENGTH
	+ MAX_DEBUG_LENGTH + DEFAULT_PID_LENGTH + 1;
public const char* debugTypeArr[5] = {
	[DEBUG_NOTICE] = "NOTICE",
	[DEBUG_WARNING] = "WARNING",
	[DEBUG_INFO] = "INFO",
	[DEBUG_ERROR] = "ERROR",
	[DEBUG_DEBUG] = "DEBUG"
};
private boolean allowDebugType[5] = {
	[DEBUG_NOTICE] = true,
	[DEBUG_WARNING] = true,
	[DEBUG_INFO] = true,
	[DEBUG_ERROR] = true,
	[DEBUG_DEBUG] = true
};
public const char* debugModeArr[3] = {
	[DEBUG_OFF] = "OFF",
	[DEBUG_NORMAL] = "NORMAL",
	[DEBUG_FILE] = "FILE"
};

public void setDebugTypePower(byte type, boolean isAllow)
{
	if (type < DEBUG_NOTICE || type < DEBUG_DEBUG) {
		return;
	}

	allowDebugType[type] = isAllow;
}

public void setDebugMode(byte mode, const char* fileName)
{
	if (mode <= DEBUG_OFF || mode > DEBUG_FILE) {
		debugMode = DEBUG_OFF;
		return;
	}

	if (mode == DEBUG_FILE) {
		if (!fileName || !(debugFd = fopen(fileName, "ab"))) {
			debugMode = DEBUG_NORMAL;
			debug(DEBUG_WARNING, "Can't set the debug file[%s]", fileName);
			return;
		}

		debugMode = DEBUG_FILE;
		return;
	} 
	
	debugMode = DEBUG_NORMAL;
}

public byte getDebugMode()
{
	return debugMode;
}

public void debug(byte type, const char* format, ...)
{
	if (type <= DEBUG_NOTICE || type > DEBUG_DEBUG) {
		type = DEBUG_NOTICE;
	}

	if (!allowDebugType[type]) {
		return;
	}
	
	if (getDebugMode() == DEBUG_OFF) {
		return;
	}

	char buf[maxBufLength];
	memset(buf, 0, maxBufLength);
	snprintf(buf, DEFAULT_PID_LENGTH, "[%d] ", getpid());
	size_t offset = strlen(buf);
	time_t rawTime;
	struct tm *timeInfo;
	time(&rawTime);
	timeInfo = localtime(&rawTime);
	strftime(buf + offset, DEFAULT_TIME_LENGTH, "[%Y-%m-%d %H:%M:%S]", timeInfo);
	offset = strlen(buf);
	snprintf(buf + offset, DEFAULT_INFO_LENGTH, " [%s] ", debugTypeArr[type]);
	offset = strlen(buf);
	size_t errorLen = 0;
	if (type == DEBUG_ERROR && errno) {
		char* strError = strerror(errno);
		errorLen = strlen(strError);
		snprintf(buf + offset, errorLen, " [%s] ", strError);
	}

	va_list args;
	va_start(args, format);
	vsnprintf(buf + offset, MAX_DEBUG_LENGTH - errorLen, format, args);
	va_end(args);
	offset = strlen(buf);
	*(buf + offset) = '\n';
	if (getDebugMode() == DEBUG_NORMAL || 
		getDebugMode() == DEBUG_FILE && !debugFd
	) {
		printf("%s", buf);
		return;
	}

	fwrite(buf, sizeof(char), offset + 1, debugFd);
	fflush(debugFd);
}

public byte setDup2(const char* fileName, int oldFd)
{
	int fd = open(fileName, O_CREAT | O_RDWR | O_APPEND, S_IRUSR | S_IWUSR );
	if (fd < 0) {
		debug(DEBUG_WARNING, "Can't open the file[%s] to set dup2[%d]", fileName, oldFd);
		return S_ERR;
	}

	int newFd = dup2(fd, oldFd);
	close(fd);
	if (newFd < 0) {
		debug(DEBUG_WARNING, "Open the file[%s], but can't set dup2[%d]", fileName, oldFd);
		return S_ERR;
	}

	return S_OK;
}

public void dropDebug()
{
	if (debugFd) {
		fclose(debugFd);
	}
}