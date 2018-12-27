/**
 * User: zhangjian2
 */

#include "process.h"
#include <string.h>
#include "debug/debug.h"
#include "malloc.h"

private int psBufferSize = 0;
private char* psBuffer;
public extern char** environ;
public int saveArgc;

#ifdef PS_SAVE_ARGV
	public static char** saveArgv;
#endif

public void processJoin(_sync* sc)
{
	if (!sc ||
		sc->type != PROCESS ||
		!sc->id ||
		!sc->start ||
		sc->start != PROCESS_MAIN_START) {
		return;
	}

	int status;
	waitpid(sc->id, &status, 0);
	debug(DEBUG_NOTICE, "The process[%d] has exited", (int)sc->id);
}

public void processStart(_sync* sc)
{
	if (!sc || sc->type != PROCESS || sc->start) {
		return;
	}

	pid_t pid = fork();
	switch (pid) {
		case -1:
			// debug
			debug(DEBUG_ERROR, "Parent[%d] can't create child process", (int)getpid());
			return;
		case 0:
			// child
			sc->id = (cid_t)getpid();
			sc->start = PROCESS_CHILD_START;
			if (sc->flag) {
				processDetach(sc);
			}

			processDeal(sc);
			deleteSync(sc);
			exit(0);
		default:
			// parent
			sc->id = (cid_t)pid;
			sc->start = PROCESS_MAIN_START;
			return;
	}
}

public void processDetach(_sync* sc)
{
	// 只能是内部调用
	if (!sc ||
		sc->type != PROCESS ||
		!sc->start ||
		sc->start != PROCESS_CHILD_START) {
		return;
	}

	pid_t pid = fork();
	switch (pid) {
		case -1:
			// debug
			debug(DEBUG_ERROR, "Parent[%d] can't create child process", (int)sc->id);
			break;
		case 0:
			// child
			sc->id = getpid();
			break;
		default:
			// parent
			deleteSync(sc);
			exit(0);
	}
}

public void* processDeal(_sync* sc)
{
	if (!sc ||
		sc->type != PROCESS ||
		!sc->args ||
		!sc->start ||
		sc->start != PROCESS_CHILD_START
	) {
		return NULL;
	}

	callSync(sc);
	return NULL;
}

public void releaseChild(childExitCallback func)
{
	int status;
	pid_t pid;
	char info[128];
	byte exit_type;
	int exit_code;
	size_t info_len = sizeof(info);
	while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
		exit_type = CHILD_EXIT_NORMAL;
		memset(info, 0, info_len);
		if (WIFEXITED(status)) {
			// 正常退出
			exit_code = WEXITSTATUS(status);
			snprintf(info, info_len, "with code: %d", exit_code);
		} else if (WIFSIGNALED(status)) {
			exit_code = WTERMSIG(status);
			snprintf(info, info_len, "with signal: %d", exit_code);
			exit_type = CHILD_EXIT_SIGNAL;
		} else if (WIFSTOPPED(status)) {
			exit_type = CHILD_EXIT_TRACE;
			exit_code = 0;
			if (func) {
				func(pid, exit_type, exit_code);
			}

			debug(DEBUG_NOTICE, "Child: %d stopped for tracing\n", (int)pid);
			continue;
		} else {
			exit_type = CHILD_EXIT_UNKNOWN;
			exit_code = 0;
			snprintf(info, info_len, "with unknown status[%d]", status);
		}

		if (func) {
			func(pid, exit_type, exit_code);
		}

		debug(DEBUG_NOTICE, "Child: %d has exited %s\n", (int)pid, info);
	}
}

public byte saveArgs(int argc, char** argv)
{
	char* endArea;
	int index = 0;
	saveArgc = argc;
	boolean isErr = false;
	for (; !isErr && index < argc; index++) {
		if (index > 0 && endArea + 1 != argv[index]) {
			isErr = true;
		}

		endArea = argv[index] + strlen(argv[index]);
	}

	for (index = 0; (!isErr) && (environ[index] != NULL); index++) {
		if (endArea + 1 != environ[index]) {
			isErr = true;
		}

		endArea = environ[index] + strlen(environ[index]);
	}

	if (isErr) {
		goto saveError;
	}

	psBuffer = argv[0];
	psBufferSize = endArea - argv[0];
	char** newEnviron = (char**)malloc((index + 1) * sizeof(char*));
	if (!newEnviron) {
		goto saveError;
	}

	for (index = 0; environ[index] != NULL; index++) {
		newEnviron[index] = strdup(environ[index]);
		if (!newEnviron[index]) {
			int i;
			for (i = 0; i < index; i++) {
				free(newEnviron[i]);
			}

			free(newEnviron);
			goto saveError;
		}
	}

	newEnviron[index] = NULL;
	environ = newEnviron;
#ifdef PS_SAVE_ARGV
	saveArgv = (char**)malloc((argc + 1) * sizeof(char*));
	if (!saveArgv) {
		goto saveError;
	}

	for (index = 0; index < argc; index++) {
		saveArgv[index] = strdup(argv[index]);
		if (!saveArgv[index]) {
			int i;
			for (i = 0; i < index; i++) {
				free(saveArgv[i]);
			}

			free(saveArgv);
			goto saveError;
		}
	}

	saveArgv[index] = NULL;
#endif
	return S_OK;

saveError:
	psBuffer = NULL;
	psBufferSize = 0;
	debug(DEBUG_WARNING, "Save script argv or environ is error");
	return S_ERR;
}

public void setProcessTitle(const char* title)
{
	if (!title) {
		return;
	}

	size_t titleLen = strlen(title);
	if (titleLen < 1 ||
		psBuffer == NULL ||
		psBufferSize < 1) {
		return;
	}

	strncpy(psBuffer, title, psBufferSize);
	if (titleLen < psBufferSize) {
		memset(psBuffer + titleLen, '\0', psBufferSize - titleLen);
	} else {
		psBuffer[psBufferSize - 1] = '\0';
	}
}

public byte setDaemonize()
{
	pid_t pid = fork();
	switch (pid) {
		case -1:
			// debug
			debug(DEBUG_ERROR, "Parent[%d] can't create child process", (int)getpid());
			return S_ERR;
		case 0:
			// child
			setsid();
			break;
		default:
			// parent
			exit(0);
	}

	return S_OK;
}