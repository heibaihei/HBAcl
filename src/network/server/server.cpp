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

// 阻塞模式
static void echo_client(ACL_VSTREAM *client)
{
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

static void run(const char *addr)
{
    const char *myname = "run";
    ACL_VSTREAM *sstream;
    ACL_VSTREAM *client=NULL;
    char  ebuf[256];
    
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
        client = acl_vstream_accept(sstream, NULL, 0);
        if (client == NULL) {
            printf("%s(%d): accept error(%s)\r\n",
                   myname, __LINE__,
                   acl_last_strerror(ebuf, sizeof(ebuf)));
            return;
        }
        printf("accept one\r\n");
        /* 获得一个客户端连接流 */
        /* 开始处理该客户端连接流 */
        echo_client(client);
    }
}

static void init(void)
{
    acl_lib_init();  /* 初始化ACL库 */
}

static void usage(const char *procname)
{
    printf("usage: %s listen_addr\r\n", procname);
    printf("example: %s 127.0.0.1:8081\r\n", procname);
}

#if 0
/** For test main */
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
