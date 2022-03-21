/*************************************************
 Copyright (c) 2022
 All rights reserved.
 File name:     wn_cgi.c
 Description:   
 History:
 1. Version:    
    Date:       2022-03-21
    Author:     WKJay
    Modify:     
*************************************************/

#include "webnet.h"
#include <dfs_posix.h>
#include <wn_module.h>
#include <wn_utils.h>
#include <stdio.h>
#include <wn_cgi.h>
#include "web_utils.h"
#include <cJSON.h>

#define DBG_TAG "wncgi"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/**
 * Name:    json_create_web_response
 * Brief:   创建一个web响应
 * Input:
 *  @code:  响应码
 *  @msg:   响应信息
 * Output:  响应体字符串
 */
static char *json_create_web_response(int code, const char *msg) {
    char *json_data = NULL;
    cJSON *root = cJSON_CreateObject();

    if (msg == RT_NULL) {
        msg = " ";
    }

    cJSON_AddItemToObject(root, "code", cJSON_CreateNumber(code));
    cJSON_AddItemToObject(root, "msg", cJSON_CreateString(msg));

    json_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_data;
}

#define cgi_head(type, code, length)                                      \
    do {                                                                  \
        const char *mimetype;                                             \
        mimetype = mime_get_type(type);                                   \
        session->request->result_code = code;                             \
        webnet_session_set_header(session, mimetype, code, "Ok", length); \
    } while (0)

void cgi_handshake(struct webnet_session *session) {
    char *json_data = NULL;
    cJSON *root = cJSON_CreateObject();
    cJSON *handshake = cJSON_CreateObject();

    cJSON_AddItemToObject(root, "code", cJSON_CreateNumber(0));
    cJSON_AddItemToObject(root, "handshake", handshake);

    cJSON_AddItemToObject(handshake, "version", cJSON_CreateString("V1.0.1"));
    cJSON_AddItemToObject(handshake, "support_firmwareupload", cJSON_CreateBool(1));
    cJSON_AddItemToObject(handshake, "support_fileupload", cJSON_CreateBool(0));
    cJSON_AddItemToObject(handshake, "support_directoryupload", cJSON_CreateBool(0));
    cJSON_AddItemToObject(handshake, "support_diskclean", cJSON_CreateBool(0));
    cJSON_AddItemToObject(handshake, "support_diskfree", cJSON_CreateBool(0));
    cJSON_AddItemToObject(handshake, "support_filecheck", cJSON_CreateBool(0));

    json_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    cgi_head("json", 200, strlen(json_data));
    webnet_session_printf(session, json_data);
    if (json_data) rt_free(json_data);
}

void cgi_diskfree(struct webnet_session *session) {
    int rs = -1;
    struct statfs buffer;
    long long total_size, free_size;
    char *json_data = NULL;
    cJSON *root = NULL, *disks = NULL, *disk = NULL;

    rs = dfs_statfs("/", &buffer);
    if (rs != 0) {
        LOG_E("dfs_statfs failed.");
        json_data = json_create_web_response(-1, "get disk free failed.");
        goto exit;
    }

    root = cJSON_CreateObject();
    disks = cJSON_CreateArray();
    disk = cJSON_CreateObject();

    cJSON_AddItemToObject(root, "code", cJSON_CreateNumber(0));
    cJSON_AddItemToObject(root, "disks", disks);
    cJSON_AddItemToArray(disks, disk);

    total_size = ((long long)buffer.f_bsize) * ((long long)buffer.f_blocks);
    free_size = ((long long)buffer.f_bsize) * ((long long)buffer.f_bfree);

    cJSON_AddItemToObject(disk, "name", cJSON_CreateString("SD"));
    cJSON_AddItemToObject(disk, "total", cJSON_CreateNumber(total_size));
    cJSON_AddItemToObject(disk, "free", cJSON_CreateNumber(free_size));

    json_data = cJSON_PrintUnformatted(root);

exit:
    cgi_head("json", 200, strlen(json_data));
    webnet_session_printf(session, json_data);
    if (json_data) rt_free(json_data);
    if (root) cJSON_Delete(root);
}
