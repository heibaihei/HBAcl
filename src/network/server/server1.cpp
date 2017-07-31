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

// 并行模式

#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>

static void run(const char *addr)
{
    const char *myname = "run";
    ACL_VSTREAM *client;
    char  ebuf[256], buf[1024];
    int   n, cnt = 0;
    
    /* 连接远程服务器，采用阻塞模式连接，连接超时为10秒，
     * 流的读超时时间为20秒，流的缓冲区大小为1024字节
     */
    client = acl_vstream_connect(addr, ACL_BLOCKING, 10, 20, 1024);
    if (client == NULL) {
        printf("%s(%d): connect addr %s error(%s)\r\n",
               myname, __LINE__, addr,
               acl_last_strerror(ebuf, sizeof(ebuf)));
        return;
    }
    
    printf("%s: connect %s ok\r\n", myname, addr);
    while (1) {
        /* 向服务器发送一行数据 */
        n = acl_vstream_fprintf(client, ">>hi, I'm coming in...(%d)\r\n", ++cnt);
        if (n == ACL_VSTREAM_EOF)
            break;
        
        /* 从服务器读取一行数据 */
        n = acl_vstream_gets(client, buf, sizeof(buf));
        if (n == ACL_VSTREAM_EOF)
            break;
        
        /* 最多循环5次 */
        if (cnt >= 5)
            break;
        
        /* 休息一下 */
        sleep(1);
    }
    /* 关闭流 */
    acl_vstream_close(client);
}

static void init(void)
{
    acl_lib_init();  /* 初始化ACL库 */
}

static void usage(const char *procname)
{
    printf("usage: %s server_addr\r\n", procname);
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
