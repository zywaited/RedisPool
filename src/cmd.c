#include "cmd.h"
#include <string.h>
#include <unistd.h>
#include "debug/debug.h"
#include <signal.h>
#include "map/tree.h"
#include "net/client.h"

#ifdef HAVE_SYS_SIGNALFD_H
#include <sys/signalfd.h>
#endif

public signalA SIGNAL_ON[MAX_SIGNAL_LEN];

public extern server* service;
public extern global* globals;

public boolean iniCmd(dictionary* ini, int argc, char** argv)
{
	if (argc <= 1) {
		return true;
	}

	char* cmd = argv[1];
	if (isStart(cmd)) {
		return true;
	}

	if (!isReStart(cmd) && !isStop(cmd)) {
		debug(DEBUG_WARNING, "the cmd not valid[%s]", cmd);
		return false;
	}

	if (!ini) {
		return false;
	}

	const char* pidfile = iniparser_getstring(ini, "global:pidfile", NULL);
	if (!pidfile || strlen(pidfile) < 1) {
		debug(DEBUG_ERROR, "the pid file not exist");
		return false;
	}

	FILE* f = fopen(pidfile, "r");
	if (!f) {
		debug(DEBUG_ERROR, "the pid file not exist");
		return false;
	}

	pid_t pid = 0;
	fscanf(f, "%d", &pid);
	fclose(f);
	if (pid <= 0) {
		return false;
	}

	debug(DEBUG_INFO, "send a signal to server[%d][%s]", pid, cmd);
	isStop(cmd) ? kill(pid, SIGTERM) : kill(pid, SIGHUP);
	return false;
}

public boolean isStart(char* cmd)
{
	return cmd && strcasecmp(cmd, START_CMD) == 0 ? true : false;
}

public boolean isReStart(char* cmd)
{
	return cmd && strcasecmp(cmd, RESTART_CMD) == 0 ? true : false;
}

public boolean isStop(char* cmd)
{
	return cmd && strcasecmp(cmd, STOP_CMD) == 0 ? true : false;
}

public void stopWorker(int signo, boolean parent)
{
	if (signo == SIGTERM || 
		signo == SIGINT ||
		signo == SIGQUIT
	) {
		globals->isExit = true;
	}

	if (!parent) {
		globals->isExit = true;
        if (service->clients->total < 1) {
		    exit(0);
		}

		return;
	}

	if (!globals->children || globals->children->total < 1) {
		return;
	}

	int pid = 0;
	mapValue* p = NULL;
	tree* children = globals->children;
	if (signo == SIGHUP) {
		tree* tmp = newTree(globals->children->max, NULL);
		if (!tmp) {
			return;
		}

		globals->children = tmp;
	}

	debug(DEBUG_INFO, "send a signal to child[num: %d, signo: %d]", children->total, signo);
	FOREACH_TREE(children, pid, p)
		if (signo == SIGHUP) {
			treePut(globals->children, (int)pid, p);
			signo = SIGTERM;
		}

		kill(pid, signo);
	END_FOREACH_TREE()
	if (signo == SIGHUP) {
		freeTree(children);
	}
}

public void stopParentWorker(int signo)
{
	stopWorker(signo, true);
}

public void stopChildWorker(int signo)
{
	stopWorker(signo, false);
}

public void installSignal(boolean parent)
{
	memset(SIGNAL_ON, 0, sizeof(SIGNAL_ON));
	sigset_t mask;
	sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGHUP);
    SIGNAL_ON[SIGTERM].signo = SIGTERM;
    SIGNAL_ON[SIGTERM].func = stopParentWorker;
    SIGNAL_ON[SIGINT].signo = SIGINT;
    SIGNAL_ON[SIGINT].func = stopParentWorker;
    SIGNAL_ON[SIGQUIT].signo = SIGQUIT;
    SIGNAL_ON[SIGQUIT].func = stopParentWorker;
    SIGNAL_ON[SIGHUP].signo = SIGHUP;
    SIGNAL_ON[SIGHUP].func = stopParentWorker;
    if (parent) {
    	struct sigaction act;
    	act.sa_handler = dealSingal;
    	act.sa_flags = 0;
    	sigaction(SIGINT, &act, 0);
    	sigaction(SIGQUIT, &act, 0);
    	sigaction(SIGTERM, &act, 0);
    	sigaction(SIGHUP, &act, 0);
    	return;
    }

    SIGNAL_ON[SIGTERM].func = stopChildWorker;
    SIGNAL_ON[SIGINT].func = stopChildWorker;
    SIGNAL_ON[SIGQUIT].func = stopChildWorker;
    SIGNAL_ON[SIGHUP].func = stopChildWorker;
#ifdef HAVE_SYS_SIGNALFD_H
    if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
    	debug(DEBUG_ERROR, "install signal error");
    	return;
    }

#ifdef SFD_NONBLOCK
    int sfd = signalfd(-1, &mask, SFD_NONBLOCK);
#else
    int sfd = signalfd(-1, &mask, 0);
    setNonBlockByFd(sfd);
#endif
    service->readSg = newClientE(readSg, EV_READ | EV_PERSIST, NULL, NULL);
    service->addEvent(service->base, sfd, service->readSg);
    return;
#endif
    struct sigaction act;
	act.sa_handler = dealSingal;
	act.sa_flags = 0;
	sigaction(SIGINT, &act, 0);
	sigaction(SIGQUIT, &act, 0);
	sigaction(SIGTERM, &act, 0);
	sigaction(SIGHUP, &act, 0);
}

public void dealSingal(int signo)
{
	debug(DEBUG_INFO, "accept a signal[%d]", signo);
	signalA sa = SIGNAL_ON[signo];
	if (sa.signo <= 0 || !sa.func) {
		return;
	}

	sa.func(signo);
}

#ifdef HAVE_SYS_SIGNALFD_H
public void readSg(evutil_socket_t fd, short flag, void* arg)
{
	struct signalfd_siginfo fdsi;
	size_t siginfoLen = sizeof(struct signalfd_siginfo);
 	size_t rLen = read(fd, &fdsi, siginfoLen);  
    if (rLen != siginfoLen) {
    	debug(DEBUG_WARNING, "read a signal error");
        return;
    }

    dealSingal((int)fdsi.ssi_signo);
}
#endif