//
//  dns.cpp
//  HBAcl
//
//  Created by zj-db0519 on 2017/7/31.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "dns.h"
#include "lib_acl.h"
#include <stdio.h>

static int dns_lookup(const char *domain, const char *dns_ip,
                      unsigned short dns_port)
{
    ACL_RES *res = NULL;  /* DNS 查询对象 */
    ACL_DNS_DB *dns_db = NULL;  /* 存储查询结果 */
    ACL_ITER iter;  /* 通用迭代对象 */
    
#define RETURN(_x_) do { \
    if (res) \
        acl_res_free(res); \
    if (dns_db) \
        acl_netdb_free(dns_db); \
    return (_x_); \
} while (0)

/* 创建一个DNS查询对象 */
res = acl_res_new(dns_ip, dns_port);

/* 向DNS服务器发送查询指令并接收处理结果 */
dns_db = acl_res_lookup(res, domain);
if (dns_db == NULL) {
    printf("failed for domain %s, %s", domain, acl_res_errmsg(res));
    RETURN (-1);
}

/* 遍历查询结构并输出至标准输出 */
printf("type\tttl\tip\t\tnet\t\tqid\t\n");
acl_foreach(iter, dns_db) {
    ACL_HOST_INFO *info;
    struct in_addr in;
    char  buf[32];
    
    info = (ACL_HOST_INFO*) iter.data;
    in.s_addr = info->saddr.sin_addr.s_addr;
    
    /* 假设网络掩码为24位，获得网络地址 */
    acl_mask_addr((unsigned char*) &in.s_addr, sizeof(in.s_addr), 24);
    
    /* 将地址转换成字符串 */
    acl_inet_ntoa(in, buf, sizeof(buf));
    
    /* 输出查询结果 */
    printf("A\t%d\t%s\t%s\t%d\r\n",
           info->ttl, info->ip, buf, res->cur_qid);
}

RETURN (0);
}

/** For test main */
#if 0
int main(int argc acl_unused, char *argv[] acl_unused)
{
    const char *dns_in_ip = "192.168.1.33", *dns_out_ip = "211.239.1.33";
    unsigned short dns_in_port = 53, dns_out_port = 53;
    const char *domain = "www.test.com.cn";
    
    /* 查询内网DNS */
    (void) dns_lookup(domain, dns_in_ip, dns_in_port);
    
    /* 查询外网DNS */
    (void) dns_lookup(domain, dns_out_ip, dns_out_port);
    
    return (0);
}
#endif
