//
//  server.hpp
//  HBAcl
//
//  Created by zj-db0519 on 2017/7/31.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef server_hpp
#define server_hpp

#include <stdio.h>
#include <stdlib.h>
#include<pthread.h>
#include<string.h>
#include<sys/types.h>
#include<unistd.h>
#include "lib_acl.h"


void* Client_Run(void *arg);

/** 阻塞式线程服务器 */
int server_test_demo();

#endif /* server_hpp */
