//
//  udp.cpp
//  HBAcl
//
//  Created by zj-db0519 on 2017/7/31.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "udp.h"
#include "lib_acl.h"

/** UDP 服务端程序 */
static void udp_server(void)
{
    const char *addr = "127.0.0.1:1088";
    char  buf[4096] = {0};
    int   ret = -1;
    ACL_VSTREAM *stream = acl_vstream_bind(addr, 0);  /* 绑定 UDP 套接口 */
    
    if (stream == NULL) {
        printf("acl_vstream_bind %s error %s\r\n", addr, acl_last_serror());
        return;
    }
    
    printf("bind udp addr %s ok\r\n", addr);
    
    while (1) {
        /* 等待客户端数据 */
        ret = acl_vstream_read(stream, buf, sizeof(buf) - 1);
        if (ret == ACL_VSTREAM_EOF) {
            printf("acl_vstream_read error %s\r\n", acl_last_serror());
            break;
        }
        
        /* 输出服务器绑定地址及远程客户端地址 */
        printf("local addr: %s, peer addr: %s, total: %d\r\n",
               ACL_VSTREAM_LOCAL(stream), ACL_VSTREAM_PEER(stream), ret);
        
        /* 回写数据至客户端 */
        ret = acl_vstream_write(stream, buf, ret);
        if (ret == ACL_VSTREAM_EOF) {
            printf("acl_vtream_writen error %s\r\n", acl_last_serror());
            break;
        }
    }
    
    /* 关闭 UDP 套接字 */
    acl_vstream_close(stream);
}

static void udp_client(void)
{
    const char *local_addr = "127.0.0.1:1089";  /* 本客户端绑定的地址 */
    const char *peer_addr = "127.0.0.1:1088";  /* 服务端绑定的地址 */
    int   i, ret, dlen;
    char  buf[1024] = {0}, data[1024] = {0};
    ACL_VSTREAM *stream = acl_vstream_bind(local_addr, 2);  /* 绑定 UDP 套接口 */
    
    if (stream == NULL) {
        printf("acl_vstream_bind %s error %s\r\n",
               local_addr, acl_last_serror());
        return;
    }
    
    memset(data, 'X', sizeof(data));
    dlen = sizeof(data);
    
    for (i = 0; i < 100; i++) {
        /* 每次写时需要设定服务端地址 */
        acl_vstream_set_peer(stream, peer_addr);
        
        /* 向服务端写入数据包 */
        ret = acl_vstream_write(stream, data, dlen);
        if (ret == ACL_VSTREAM_EOF) {
            printf("acl_vtream_writen error %s\r\n",
                   acl_last_serror());
            break;
        }
        
        /* 从服务端读取数据 */
        ret = acl_vstream_read(stream, buf, sizeof(buf));
        if (ret == ACL_VSTREAM_EOF) {
            printf("acl_vstream_read error %s\r\n",
                   acl_last_serror());
            break;
        }
    }
    
    /* 关闭客户端 UDP 套接字 */
    acl_vstream_close(stream);
}
