//
//  server.cpp
//  HBAcl
//
//  Created by zj-db0519 on 2017/7/31.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "server.h"

// 阻塞模式
void* echo_client(ACL_VSTREAM *client)
{
    char  buf[1024]={0};
    int   dataLen=0;
    
    /* 设置客户端流的读超时时间为30秒 */
    ACL_VSTREAM_SET_RWTIMO(client, 30);
    
    /* 循环读客户端的数据，直到其关闭或出错或超时 */
    while (true) {
        /* 等待读客户端发来的数据 */
        dataLen = acl_vstream_read(client, buf, sizeof(buf));
        if (dataLen == ACL_VSTREAM_EOF)
            break;
        
        printf("[Svr rev]:%s \r\n", buf);
        
        /* 将读到的数据写回至客户端流 */
        if (acl_vstream_writen(client, buf, dataLen) == ACL_VSTREAM_EOF)
            break;
    }
    
    /* 关闭客户端流 */
    acl_vstream_close(client);
    return NULL;
}

static void* Server_Run(void * arg)
{
    const char *addr = (char *)arg;
    const char *myname = "阻塞式线程服务器";
    ACL_VSTREAM *sstream=NULL;
    ACL_VSTREAM *client=NULL;
    char  ebuf[256];
    
    /* 监听一个本地地址 */
    sstream = acl_vstream_listen(addr, 128);
    if (sstream == NULL) {
        printf("%s(%d): listen on %s error(%s)\r\n", myname, __LINE__, addr, acl_last_strerror(ebuf, sizeof(ebuf)));
        return NULL;
    }
    
    printf("%s: listen %s ok\r\n", myname, addr);
    while (1) {
        /* 等待接受客户端的连接 */
        client = acl_vstream_accept(sstream, NULL, 0);
        if (client == NULL) {
            printf("%s(%d): accept error(%s)\r\n",
                   myname, __LINE__,
                   acl_last_strerror(ebuf, sizeof(ebuf)));
            return NULL;
        }
        printf("accept one\r\n");
        /* 获得一个客户端连接流 */
        /* 开始处理该客户端连接流 */
        echo_client(client);
    }
    return NULL;
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

void* Client_Run(void *arg)
{
    const char *addr = (char *)arg;
    const char *myname = "用户1<客户端>";
    ACL_VSTREAM *client = NULL;
    char  ebuf[256]={0}, buf[1024]={0};
    int   n, cnt = 0;
    
    /* 连接远程服务器，采用阻塞模式连接，连接超时为10秒，
     * 流的读超时时间为20秒，流的缓冲区大小为1024字节
     */
    client = acl_vstream_connect(addr, ACL_BLOCKING, 10, 20, 1024);
    if (client == NULL) {
        printf("%s(%d): connect addr %s error(%s)\r\n", myname, __LINE__, addr, acl_last_strerror(ebuf, sizeof(ebuf)));
        return NULL;
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
        
        printf("[Client rev]:%s \r\n", buf);
        
        /* 最多循环5次 */
        if (cnt >= 5)
            break;
        
        /* 休息一下 */
        sleep(1);
    }
    /* 关闭流 */
    acl_vstream_close(client);
    return NULL;
}

int server_test_demo()
{
    int argc = 2;
    char *listen_addr = (char *)"127.0.0.1:8080";
    char *local_addr = (char *)"127.0.0.1:8080";
    
    if (argc != 2) {
        usage(listen_addr);
        return (0);
    }
    
    init();
    
    pthread_t gSvrThread;
    int threadSvrID = pthread_create(&gSvrThread, NULL, Server_Run, local_addr);
    if(threadSvrID != 0) {
        printf("can't create server thread: %s\n",strerror(threadSvrID));
        return 1;
    }
    
    sleep(3);
    
    pthread_t gCliThread;
    int threadCliID = pthread_create(&gCliThread, NULL, Client_Run, listen_addr);
    if(threadCliID != 0) {
        printf("can't create server thread: %s\n", strerror(threadCliID));
        return 1;
    }
    
    while (true) {
        sleep(20);
    }
    return (0);
}
