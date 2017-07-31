//
//  server.cpp
//  HBAcl
//
//  Created by zj-db0519 on 2017/7/31.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "server.hpp"

#include "lib_acl.h"  /* 先包含ACL库头文件 */
#include <stdio.h>
#include <stdlib.h>

// 线程池

#include "lib_acl.h"  /* 先包含ACL库头文件 */
#include <stdio.h>
#include <stdlib.h>

/**
 * 单独的线程处理来自于客户端的连接
 * @param arg {void*} 添加任务时的对象
 */
static void echo_client_thread(void *arg)
{
    ACL_VSTREAM *client = (ACL_VSTREAM*) arg;
    char  buf[1024];
    int   n;
    
    /* 设置客户端流的读超时时间为30秒 */
    ACL_VSTREAM_SET_RWTIMO(client, 30);
    
    /* 循环读客户端的数据，直到其关闭或出错或超时 */
    while (1) {
        /* 等待读客户端发来的数据 */
        n = acl_vstream_read(client, buf, sizeof(buf));
        if (n == ACL_VSTREAM_EOF)
            break;
        /* 将读到的数据写回至客户端流 */
        if (acl_vstream_writen(client, buf, n) == ACL_VSTREAM_EOF)
            break;
    }
    
    /* 关闭客户端流 */
    acl_vstream_close(client);
}

/**
 * 创建半驻留线程池的过程
 * @return {acl_pthread_pool_t*} 新创建的线程池句柄
 */
static acl_pthread_pool_t *create_thread_pool(void)
{
    acl_pthread_pool_t *thr_pool;  /* 线程池句柄 */
    int  max_threads = 100;  /* 最多并发100个线程 */
    int  idle_timeout = 10;  /* 每个工作线程空闲10秒后自动退出 */
    acl_pthread_pool_attr_t attr;  /* 线程池初始化时的属性 */
    
    /* 初始化线程池对象属性 */
    acl_pthread_pool_attr_init(&attr);
    acl_pthread_pool_attr_set_threads_limit(&attr, max_threads);
    acl_pthread_pool_attr_set_idle_timeout(&attr, idle_timeout);
    
    /* 创建半驻留线程句柄 */
    thr_pool = acl_pthread_pool_create(&attr);
    assert(thr_pool);
    return (thr_pool);
}

/**
 * 开始运行
 * @param addr {const char*} 服务器监听地址，如：127.0.0.1:8081
 */
static void run(const char *addr)
{
    const char *myname = "run";
    acl_pthread_pool_t *thr_pool;
    ACL_VSTREAM *sstream;
    char  ebuf[256];
    
    thr_pool = create_thread_pool();
    
    /* 监听一个本地地址 */
    sstream = acl_vstream_listen(addr, 128);
    if (sstream == NULL) {
        printf("%s(%d): listen on %s error(%s)\r\n",
               myname, __LINE__, addr,
               acl_last_strerror(ebuf, sizeof(ebuf)));
        return;
    }
    
    printf("%s: listen %s ok\r\n", myname, addr);
    while (1) {
        /* 等待接受客户端的连接 */
        ACL_VSTREAM *client = acl_vstream_accept(sstream, NULL, 0);
        if (client == NULL) {
            printf("%s(%d): accept error(%s)\r\n",
                   myname, __LINE__,
                   acl_last_strerror(ebuf, sizeof(ebuf)));
            break;
        }
        printf("accept one\r\n");
        /* 获得一个客户端连接流 */
        /* 开始处理该客户端连接流 */
        
        /**
         * 向线程池中添加一个任务
         * @param thr_pool 线程池句柄
         * @param echo_client_thread 工作线程的回调函数
         * @param client 客户端数据流
         */
        acl_pthread_pool_add(thr_pool, echo_client_thread, client);
    }
    
    /* 销毁线程池对象 */
    acl_pthread_pool_destroy(thr_pool);
}

/**
 * 初始化过程
 */
static void init(void)
{
    /* 初始化ACL库 */
    acl_lib_init();
}

/**
 * 使用提示接口
 * @param procname {cosnt char*} 程序名
 */
static void usage(const char *procname)
{
    printf("usage: %s listen_addr\r\n", procname);
    printf("example: %s 127.0.0.1:8081\r\n", procname);
    getchar();
}

#if 0
int main(int argc, char *argv[])
{
    if (argc != 2) {
        usage(argv[0]);
        return (0);
    }
    
    init();
    run(argv[1]);
    return (0);
}
#endif
