#ifndef __SERVER_HEADER
#define __SERVER_HEADER

#ifdef USE_EPOLL
// #include "event/epoll.h"
#else
#include "event/libevent.h"
#endif

#include "debug/debug.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include "client.h"
#include "pool.h"
#include "types.h"
#include <unistd.h>
#include <libgen.h>
#include <pthread.h>
#include "proto/multibulk.h"
#include "s.h"
#include "sync/thread.h"
#include "sync/process.h"
#include <math.h>

#define MAX_INFO_NUM 5
#define CONFIG_NAME "rp.ini"
#define FORK_MAX_NUM 20

#define SOCK_CH_FD 1
#define SOCK_CH_RN 2

public global* globals;
public server* service;
public pool* rps;

/**
 * @access public
 */
public void run(int, char**);

/**
 * @param int fd
 * @param size_t maxClientSize
 * @param size_t millisecond
 */
public boolean initServer(int, size_t, size_t);

/**
 * @access public
 * @param server* service
 */
public void freeServer(server*);

/**
 * @param evutil_socket_t fd
 * @param short flag
 * @param void* args
 */
public void readR(evutil_socket_t, short, void*);

/**
 * @param evutil_socket_t fd
 * @param short flag
 * @param void* args
 */
public void readC(evutil_socket_t, short, void*);

/**
 * @param evutil_socket_t fd
 * @param short flag
 * @param void* args
 */
public void writeR(evutil_socket_t, short, void*);

/**
 * @param evutil_socket_t fd
 * @param short flag
 * @param void* args
 */
public void writeC(evutil_socket_t, short, void*);

/**
 * @return int
 */
public int getTcpFd(const char*, size_t, size_t);

/**
 * @return int
 */
public int getUnixFd(const char*, size_t);

/**
 * check process exit and client timeout
 * @param evutil_socket_t fd
 * @param short flag
 * @param void* args
 */
public void checkC(evutil_socket_t, short, void*);

/**
 * @param evutil_socket_t fd
 * @param short flag
 * @param void* args
 */
public void acceptC(evutil_socket_t, short, void*);

/**
 * 连接socket
 * @return int
 */
public int connectTcpFd(char*, size_t);

/**
 * @return int
 */
public int connectUnixFd(char*);

public boolean setTimeOut(int, struct timeval*);

public void removeRC(redis*, mapKey*, client*, mapKey*);
public void removeByR(redis*, mapKey*);
public void removeByC(client*, mapKey*);

public void readS(evutil_socket_t, short, void*);

public void writeS(evutil_socket_t, short, void*);

public void updateMin();

public sock* newSock(uint32_t, struct timeval);

public void freeSock(sock*);

public void* worker(cid_t, char*, void*);

public void* task(cid_t, char*, void*);

public void* restartWorker(pid_t, byte, int);

#endif