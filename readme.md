# 项目
- 1 redis协议
- 2 网络IO
- 3 数据结构、多进程、多线程
- 4 PHP HashTable

# 依赖库
- 1 libevent
- 2 iniparser
- 3 m (math)
- 4 pthread

# 结构
<pre>
├── benchmark
├── bin 						# 执行文件目录
├── conf
│   └── rp.ini 					# 配置文件
├── examples
│   └── test.php
├── src 						# 源代码
│   ├── base.h
│   ├── cmd.c 					# 信号处理
│   ├── cmd.h
│   ├── config.h.in 			# 支持列表
│   ├── configure 				# 安装
│   ├── configure.in
│   ├── crc32.c
│   ├── crc32.h
│   ├── debug 					# 日志
│   │   ├── debug.c
│   │   └── debug.h
│   ├── event 					# event
│   │   ├── e.h
│   │   ├── libevent.c
│   │   └── libevent.h
│   ├── main.c 					# 入口
│   ├── Makefile.in 			# Makefile
│   ├── malloc.h
│   ├── map
│   │   ├── hash.c 				# hash实现（类似php HashTable, 不同处这里是无序的）
│   │   ├── hash.h
│   │   ├── map.c
│   │   ├── map.h
│   │   ├── tree.c 				# 红黑树
│   │   └── tree.h
│   ├── net
│   │   ├── client.c 			# 对fd的相关处理
│   │   ├── client.h
│   │   ├── pool.c 				# 连接池相关处理
│   │   ├── pool.h
│   │   ├── s.h
│   │   ├── server.c 			# 服务端相关处理
│   │   └── server.h
│   ├── proto
│   │   ├── multibulk.c 		# redis协议处理
│   │   └── multibulk.h
│   ├── rstack.c 				# 栈
│   ├── rstack.h
│   ├── sync
│   │   ├── interface.c
│   │   ├── interface.h
│   │   ├── process.c 			# 多进程
│   │   ├── process.h
│   │   ├── thread.c 			# 多线程
│   │   └── thread.h
│   ├── types.c 				# 基础数据
│   └── types.h
└── vendor 						# 第三方库(iniparser)                        
</pre>

# 流程
- 1 主进程创建socke, 创建管道（用于子进程redis连接数同步）
- 2 子进程： 两个线程
- 2.1 阻塞读取socket连接
- 2.2 IO复用（同步各子进程redis连接数，客户端数据传输，redis数据传输）

# 安装说明
```sh
# 下载iniparser，放入vendor中，编译
cd RedisPool/vendor/iniparser
make

# 编译安装RedisPool
cd RedisPool/src
# prefix 默认 RedisPool/bin 
# config-path 默认 RedisPool/conf
chmod +x ./configure && ./configure LDFLAGS=-L../vendor/iniparser # [--prefix=/path --with-config-path=/path]
make && make install

# 执行
RedisPool/bin/server
```

# 实例
```php
<?php
$r = new Redis();
$r->connect('/tmp/socket/rs.sock');
//var_dump($r->auth('123456'));
var_dump($r->select(0));
var_dump($r->get('waited'));
var_dump($r->get('waited_test'));

/** 返回结果(redis 设置了waited值为1)
	bool(true)
	string(1) "1"
	bool(false)
*/
```

# 注
  该项目编译通过并且能正常使用，数据处理逻辑存在一定BUG，待调整

# 待完成
- 定时检查socket连接状态