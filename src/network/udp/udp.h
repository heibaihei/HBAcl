//
//  udp.hpp
//  HBAcl
//
//  Created by zj-db0519 on 2017/7/31.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef udp_hpp
#define udp_hpp

#include<stdio.h>
#include<pthread.h>
#include<string.h>
#include<sys/types.h>
#include<unistd.h>

void* udp_client(void* arg);
void* udp_server(void *arg);

int demo();

#endif /* udp_hpp */
