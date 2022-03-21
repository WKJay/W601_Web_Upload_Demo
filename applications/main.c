/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-09-01     ZeroFree     first implementation
 * 2022-3-21      WKJay        根据本项目修改原 demo 
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <rthw.h>
#include <msh.h>

#include "drv_wifi.h"
#include "wifi_config.h"

#include <stdio.h>
#include <stdlib.h>

#include <dfs_fs.h>
#include <dfs_posix.h>

#include "web.h"

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

static struct rt_semaphore net_ready;
extern uint8_t firm_upload_done;

/**
 * The callback of network ready event
 */
static void wlan_ready_handler(int event, struct rt_wlan_buff *buff, void *parameter) {
    rt_sem_release(&net_ready);
}

int main(void) {
    int result = RT_EOK;

    /* 挂载文件系统 */
    if (dfs_mount("sd0", "/", "elm", 0, 0) == 0) {
        LOG_I("Filesystem initialized!");
    } else {
        LOG_E("Failed to initialize filesystem!");
    }

    /* 配置 wifi 工作模式 */
    rt_wlan_set_mode(RT_WLAN_DEVICE_STA_NAME, RT_WLAN_STATION);

    /* 创建信号量 */
    rt_sem_init(&net_ready, "net_ready", 0, RT_IPC_FLAG_FIFO);

    /* 注册 WIFI 连接成功回调函数 */
    rt_wlan_register_event_handler(RT_WLAN_EVT_READY, wlan_ready_handler, RT_NULL);

    /* 配置 WIFI 自动连接 */
    wlan_autoconnect_init();

    /* 使能 WIFI 自动连接 */
    rt_wlan_config_autoreconnect(RT_TRUE);

    /* 等待连接成功 */
    result = rt_sem_take(&net_ready, RT_WAITING_FOREVER);
    if (result != RT_EOK) {
        LOG_E("Wait net ready failed!");
        return -RT_ERROR;
    }

    /* 启动 webnet demo */
    webnet_create();

    while (1) {
        if (firm_upload_done) {
            LOG_W("catch firmware upload done flag, system will reset in 1 second to upgrade");
            rt_thread_mdelay(1000);
            rt_hw_cpu_reset();
        }
        rt_thread_mdelay(1000);
    }
}
