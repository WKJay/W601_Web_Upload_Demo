/*************************************************
 Copyright (c) 2022
 All rights reserved.
 File name:     web.c
 Description:   
 History:
 1. Version:    
    Date:       2022-03-21
    Author:     WKJay
    Modify:     
*************************************************/

#include "web.h"
#include "web_utils.h"
#include <webnet.h>
#include <wn_module.h>
#include <wn_cgi.h>
#include <dfs_fs.h>

#define DBG_TAG "web"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

static void _init_thread(void *param) {
    /* add firmware upload entry */
    webnet_upload_add(&upload_bin_upload);

    webnet_cgi_register("handshake", cgi_handshake);
    webnet_cgi_register("get_diskfree", cgi_diskfree);

    /* start WebNet */
    web_file_init();
    webnet_init();
}

/**
 * Name:        webnet_create
 * Brief:       初始化webnet
 * Input:       none
 * Output:      RT_EOK:成功
 */
int webnet_create(void) {
    rt_thread_t tid = rt_thread_create("web_init", _init_thread, NULL, 8192, 5, 5);
    if (tid) {
        rt_thread_startup(tid);
        return 0;
    } else {
        return -1;
    }
}
